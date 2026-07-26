#define _GNU_SOURCE
#include "steamkit/steam/handlers/steam_content.h"
#include "steamkit/steam/handlers/client_msg_handler.h"
#include "steamkit/steam/handlers/steam_unified_messages.h"
#include "steamkit/steam/steam_client.h"
#include "steamkit/steam/callbacks.h"
#include "steamkit/utils/debug_log.h"
#include "steamkit/cdn/cdn_server.h"
#include "steammessages_contentsystem.steamclient.pb-c.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

static char* sk_strdup(const char* s) {
    if (!s) return NULL;
    size_t len = strlen(s) + 1;
    char* dup = (char*)malloc(len);
    if (dup) memcpy(dup, s, len);
    return dup;
}

static void sk_steam_content_handle_msg(struct sk_client_msg_handler* handler, const sk_packet_msg_t* packet_msg) {
    (void)handler;
    (void)packet_msg;
}

typedef struct sk_steam_content {
    struct sk_client_msg_handler base;
} sk_steam_content_t;

sk_steam_content_t* sk_steam_content_create(void) {
    sk_steam_content_t* content = (sk_steam_content_t*)calloc(1, sizeof(sk_steam_content_t));
    if (content) {
        content->base.handle_msg = sk_steam_content_handle_msg;
        content->base.handler_type = SK_HANDLER_STEAM_CONTENT;
    }
    return content;
}

void sk_steam_content_destroy(sk_steam_content_t* content) {
    free(content);
}

static sk_steam_unified_messages_t* sk_steam_content_get_unified(sk_steam_content_t* content) {
    if (!content || !content->base.client) return NULL;
    return (sk_steam_unified_messages_t*)sk_steam_client_get_handler(content->base.client, SK_HANDLER_STEAM_UNIFIED_MESSAGES);
}

typedef struct sk_content_req_ctx {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    bool done;
    sk_cdn_server_list_callback_t* server_result;
    sk_manifest_request_code_callback_t* manifest_result;
    sk_cdn_auth_token_callback_t* auth_result;
} sk_content_req_ctx_t;

static void sk_content_wait(sk_content_req_ctx_t* ctx) {
    pthread_mutex_lock(&ctx->mutex);
    while (!ctx->done) {
        pthread_cond_wait(&ctx->cond, &ctx->mutex);
    }
    pthread_mutex_unlock(&ctx->mutex);
}

static void sk_content_server_response_cb(void* user_data, const uint8_t* body, size_t body_len, uint32_t eresult) {
    sk_content_req_ctx_t* ctx = (sk_content_req_ctx_t*)user_data;
    if (eresult != 0) {
        sk_debug_log_warn("SteamContent", "GetServersForSteamPipe returned eresult=%u", eresult);
    }
    if (body && body_len > 0) {
        CContentServerDirectoryGetServersForSteamPipeResponse* resp = ccontent_server_directory__get_servers_for_steam_pipe__response__unpack(NULL, body_len, body);
        if (resp && resp->servers) {
            sk_cdn_server_t** servers = (sk_cdn_server_t**)calloc(resp->n_servers, sizeof(sk_cdn_server_t*));
            if (servers) {
                for (size_t i = 0; i < resp->n_servers; ++i) {
                    CContentServerDirectoryServerInfo* info = resp->servers[i];
                    sk_cdn_server_t* server = (sk_cdn_server_t*)calloc(1, sizeof(sk_cdn_server_t));
                    if (!server) continue;
                    server->type = sk_strdup(info->type);
                    server->host = sk_strdup(info->host);
                    server->vhost = sk_strdup(info->vhost);
                    server->port = (info->https_support && strcmp(info->https_support, "mandatory") == 0) ? 443 : 80;
                    server->cell_id = info->has_cell_id ? (uint32_t)info->cell_id : 0;
                    server->load = info->has_load ? (int)info->load : 0;
                    server->weighted_load = info->has_weighted_load ? info->weighted_load : 0.0f;
                    server->num_entries = info->has_num_entries_in_client_list ? (int)info->num_entries_in_client_list : 0;
                    server->use_as_proxy = info->has_use_as_proxy ? info->use_as_proxy : false;
                    server->proxy_request_path_template = sk_strdup(info->proxy_request_path_template);
                    server->https_support = sk_strdup(info->https_support);
                    if (info->n_allowed_app_ids > 0 && info->allowed_app_ids) {
                        server->allowed_app_ids = (uint32_t*)malloc(info->n_allowed_app_ids * sizeof(uint32_t));
                        if (server->allowed_app_ids) {
                            memcpy(server->allowed_app_ids, info->allowed_app_ids, info->n_allowed_app_ids * sizeof(uint32_t));
                            server->num_allowed_app_ids = (uint32_t)info->n_allowed_app_ids;
                        }
                    }
                    servers[i] = server;
                }
                ctx->server_result = sk_cdn_server_list_callback_create(servers, (uint32_t)resp->n_servers);
                for (size_t i = 0; i < resp->n_servers; ++i) {
                    sk_cdn_server_destroy(servers[i]);
                }
                free(servers);
            }
            ccontent_server_directory__get_servers_for_steam_pipe__response__free_unpacked(resp, NULL);
        }
    }
    pthread_mutex_lock(&ctx->mutex);
    ctx->done = true;
    pthread_cond_signal(&ctx->cond);
    pthread_mutex_unlock(&ctx->mutex);
}

static void sk_content_manifest_response_cb(void* user_data, const uint8_t* body, size_t body_len, uint32_t eresult) {
    sk_content_req_ctx_t* ctx = (sk_content_req_ctx_t*)user_data;
    if (eresult != 0) {
        sk_debug_log_warn("SteamContent", "GetManifestRequestCode returned eresult=%u", eresult);
    }
    if (body && body_len > 0) {
        CContentServerDirectoryGetManifestRequestCodeResponse* resp = ccontent_server_directory__get_manifest_request_code__response__unpack(NULL, body_len, body);
        if (resp) {
            ctx->manifest_result = sk_manifest_request_code_callback_create(0, resp->manifest_request_code);
            ccontent_server_directory__get_manifest_request_code__response__free_unpacked(resp, NULL);
        }
    }
    pthread_mutex_lock(&ctx->mutex);
    ctx->done = true;
    pthread_cond_signal(&ctx->cond);
    pthread_mutex_unlock(&ctx->mutex);
}

static void sk_content_auth_response_cb(void* user_data, const uint8_t* body, size_t body_len, uint32_t eresult) {
    sk_content_req_ctx_t* ctx = (sk_content_req_ctx_t*)user_data;
    if (eresult != 0) {
        sk_debug_log_warn("SteamContent", "GetCDNAuthToken returned eresult=%u", eresult);
    }
    if (body && body_len > 0) {
        CContentServerDirectoryGetCDNAuthTokenResponse* resp = ccontent_server_directory__get_cdnauth_token__response__unpack(NULL, body_len, body);
        if (resp) {
            ctx->auth_result = sk_cdn_auth_token_callback_create(0, resp->token);
            ccontent_server_directory__get_cdnauth_token__response__free_unpacked(resp, NULL);
        }
    }
    pthread_mutex_lock(&ctx->mutex);
    ctx->done = true;
    pthread_cond_signal(&ctx->cond);
    pthread_mutex_unlock(&ctx->mutex);
}

sk_cdn_server_list_callback_t* sk_steam_content_get_servers_for_steam_pipe(sk_steam_content_t* content) {
    if (!content || !content->base.client) return NULL;

    sk_steam_unified_messages_t* um = sk_steam_content_get_unified(content);
    if (!um) return NULL;

    sk_unified_service_t* svc = sk_steam_unified_messages_create_service(um, "ContentServerDirectory");
    if (!svc) return NULL;

    CContentServerDirectoryGetServersForSteamPipeRequest req = CCONTENT_SERVER_DIRECTORY__GET_SERVERS_FOR_STEAM_PIPE__REQUEST__INIT;
    req.cell_id = 0;
    req.has_cell_id = true;

    size_t packed_size = ccontent_server_directory__get_servers_for_steam_pipe__request__get_packed_size(&req);
    uint8_t* packed_buf = (uint8_t*)malloc(packed_size);
    if (!packed_buf) return NULL;
    ccontent_server_directory__get_servers_for_steam_pipe__request__pack(&req, packed_buf);

    sk_content_req_ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    pthread_mutex_init(&ctx.mutex, NULL);
    pthread_cond_init(&ctx.cond, NULL);

    sk_steam_unified_messages_send_request(um, svc, "GetServersForSteamPipe", packed_buf, packed_size, 0,
        sk_content_server_response_cb, &ctx);

    free(packed_buf);
    sk_content_wait(&ctx);

    pthread_mutex_destroy(&ctx.mutex);
    pthread_cond_destroy(&ctx.cond);

    if (!ctx.server_result) {
        sk_debug_log_warn("SteamContent", "GetServersForSteamPipe timed out or failed");
    }
    return ctx.server_result;
}

sk_manifest_request_code_callback_t* sk_steam_content_get_manifest_request_code(
    sk_steam_content_t* content, uint32_t depot_id, uint32_t app_id, uint64_t manifest_id, const char* branch) {
    if (!content || !content->base.client) return NULL;

    sk_steam_unified_messages_t* um = sk_steam_content_get_unified(content);
    if (!um) return NULL;

    sk_unified_service_t* svc = sk_steam_unified_messages_create_service(um, "ContentServerDirectory");
    if (!svc) return NULL;

    CContentServerDirectoryGetManifestRequestCodeRequest req = CCONTENT_SERVER_DIRECTORY__GET_MANIFEST_REQUEST_CODE__REQUEST__INIT;
    req.app_id = app_id;
    req.has_app_id = true;
    req.depot_id = depot_id;
    req.has_depot_id = true;
    req.manifest_id = manifest_id;
    req.has_manifest_id = true;
    if (branch) {
        req.app_branch = sk_strdup(branch);
    }

    size_t packed_size = ccontent_server_directory__get_manifest_request_code__request__get_packed_size(&req);
    uint8_t* packed_buf = (uint8_t*)malloc(packed_size);
    if (!packed_buf) {
        free((void*)req.app_branch);
        return NULL;
    }
    ccontent_server_directory__get_manifest_request_code__request__pack(&req, packed_buf);

    sk_content_req_ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    pthread_mutex_init(&ctx.mutex, NULL);
    pthread_cond_init(&ctx.cond, NULL);

    sk_steam_unified_messages_send_request(um, svc, "GetManifestRequestCode", packed_buf, packed_size, app_id,
        sk_content_manifest_response_cb, &ctx);

    free(packed_buf);
    free((void*)req.app_branch);
    sk_content_wait(&ctx);

    pthread_mutex_destroy(&ctx.mutex);
    pthread_cond_destroy(&ctx.cond);

    if (!ctx.manifest_result) {
        sk_debug_log_warn("SteamContent", "GetManifestRequestCode timed out or failed");
    }
    return ctx.manifest_result;
}

sk_cdn_auth_token_callback_t* sk_steam_content_get_cdn_auth_token(
    sk_steam_content_t* content, uint32_t app_id, uint32_t depot_id, const char* host) {
    if (!content || !content->base.client || !host) return NULL;

    sk_steam_unified_messages_t* um = sk_steam_content_get_unified(content);
    if (!um) return NULL;

    sk_unified_service_t* svc = sk_steam_unified_messages_create_service(um, "ContentServerDirectory");
    if (!svc) return NULL;

    CContentServerDirectoryGetCDNAuthTokenRequest req = CCONTENT_SERVER_DIRECTORY__GET_CDNAUTH_TOKEN__REQUEST__INIT;
    req.app_id = app_id;
    req.has_app_id = true;
    req.depot_id = depot_id;
    req.has_depot_id = true;
    req.host_name = sk_strdup(host);

    size_t packed_size = ccontent_server_directory__get_cdnauth_token__request__get_packed_size(&req);
    uint8_t* packed_buf = (uint8_t*)malloc(packed_size);
    if (!packed_buf) {
        free((void*)req.host_name);
        return NULL;
    }
    ccontent_server_directory__get_cdnauth_token__request__pack(&req, packed_buf);

    sk_content_req_ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    pthread_mutex_init(&ctx.mutex, NULL);
    pthread_cond_init(&ctx.cond, NULL);

    sk_steam_unified_messages_send_request(um, svc, "GetCDNAuthToken", packed_buf, packed_size, app_id,
        sk_content_auth_response_cb, &ctx);

    free(packed_buf);
    free((void*)req.host_name);
    sk_content_wait(&ctx);

    pthread_mutex_destroy(&ctx.mutex);
    pthread_cond_destroy(&ctx.cond);

    if (!ctx.auth_result) {
        sk_debug_log_warn("SteamContent", "GetCDNAuthToken timed out or failed");
    }
    return ctx.auth_result;
}
