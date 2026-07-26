#define _GNU_SOURCE
#include "steamkit/steam/handlers/steam_cloud.h"
#include "steamkit/steam/handlers/client_msg_handler.h"
#include "steamkit/steam/handlers/steam_unified_messages.h"
#include "steamkit/steam/steam_client.h"
#include "steamkit/steam/callbacks.h"
#include "steamkit/utils/debug_log.h"
#include "steamkit/base/generated/steam_msg_cloud.h"
#include "steammessages_cloud.steamclient.pb-c.h"
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

typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    bool done;
    sk_ugc_details_callback_t* result;
    sk_ccloud_enumerate_user_files_response_t* enumerate_result;
    sk_ccloud_client_file_download_response_t* file_download_result;
} sk_cloud_req_ctx_t;

static void sk_cloud_response_cb(void* user_data, const uint8_t* body, size_t body_len, uint32_t eresult) {
    (void)eresult;
    sk_cloud_req_ctx_t* ctx = (sk_cloud_req_ctx_t*)user_data;
    if (body && body_len > 0) {
        CCloudGetFileDetailsResponse* resp = ccloud__get_file_details__response__unpack(NULL, body_len, body);
        if (resp && resp->details) {
            CCloudUserFile* details = resp->details;
            ctx->result = sk_ugc_details_callback_create(
                details->has_ugcid ? details->ugcid : 0,
                details->filename,
                details->url,
                details->has_file_size ? details->file_size : 0
            );
            ccloud__get_file_details__response__free_unpacked(resp, NULL);
        }
    }
    pthread_mutex_lock(&ctx->mutex);
    ctx->done = true;
    pthread_cond_signal(&ctx->cond);
    pthread_mutex_unlock(&ctx->mutex);
}

static void sk_cloud_enumerate_response_cb(void* user_data, const uint8_t* body, size_t body_len, uint32_t eresult) {
    (void)eresult;
    sk_cloud_req_ctx_t* ctx = (sk_cloud_req_ctx_t*)user_data;
    if (body && body_len > 0) {
        CCloudEnumerateUserFilesResponse* resp = ccloud__enumerate_user_files__response__unpack(NULL, body_len, body);
        if (resp) {
            sk_ccloud_enumerate_user_files_response_t* result = (sk_ccloud_enumerate_user_files_response_t*)calloc(1, sizeof(*result));
            if (result) {
                result->num_files = resp->n_files;
                result->total_files = resp->has_total_files ? resp->total_files : resp->n_files;
                if (resp->n_files > 0 && resp->files) {
                    result->files = (sk_ccloud_user_file_t*)calloc(resp->n_files, sizeof(sk_ccloud_user_file_t));
                    if (result->files) {
                        for (size_t i = 0; i < resp->n_files; ++i) {
                            CCloudUserFile* file = resp->files[i];
                            sk_ccloud_user_file_t* sk_file = &result->files[i];
                            sk_file->filename = file->filename ? strdup(file->filename) : NULL;
                            sk_file->ugc_id = file->has_ugcid ? file->ugcid : 0;
                            sk_file->timestamp = file->has_timestamp ? file->timestamp : 0;
                            sk_file->file_size = file->has_file_size ? file->file_size : 0;
                            sk_file->url = file->url ? strdup(file->url) : NULL;
                            sk_file->steamid_creator = file->has_steamid_creator ? file->steamid_creator : 0;
                            sk_file->appid = file->has_appid ? file->appid : 0;
                        }
                    }
                }
            }
            ccloud__enumerate_user_files__response__free_unpacked(resp, NULL);
        }
    }
    pthread_mutex_lock(&ctx->mutex);
    ctx->done = true;
    pthread_cond_signal(&ctx->cond);
    pthread_mutex_unlock(&ctx->mutex);
}

static void sk_cloud_file_download_response_cb(void* user_data, const uint8_t* body, size_t body_len, uint32_t eresult) {
    (void)eresult;
    sk_cloud_req_ctx_t* ctx = (sk_cloud_req_ctx_t*)user_data;
    if (body && body_len > 0) {
        CCloudClientFileDownloadResponse* resp = ccloud__client_file_download__response__unpack(NULL, body_len, body);
        if (resp) {
            sk_ccloud_client_file_download_response_t* result = (sk_ccloud_client_file_download_response_t*)calloc(1, sizeof(*result));
            if (result) {
                result->use_https = resp->use_https;
                result->url_host = resp->url_host ? strdup(resp->url_host) : NULL;
                result->url_path = resp->url_path ? strdup(resp->url_path) : NULL;
                result->file_size = resp->has_file_size ? resp->file_size : 0;
                result->raw_file_size = resp->has_raw_file_size ? resp->raw_file_size : 0;
                result->encrypted = resp->has_encrypted ? resp->encrypted : false;
                result->timestamp = resp->has_time_stamp ? resp->time_stamp : 0;
                result->is_explicit_delete = resp->has_is_explicit_delete ? resp->is_explicit_delete : false;
                if (resp->n_request_headers > 0 && resp->request_headers) {
                    result->num_request_headers = resp->n_request_headers;
                    result->request_headers = (sk_ccloud_request_header_t*)calloc(resp->n_request_headers, sizeof(sk_ccloud_request_header_t));
                    if (result->request_headers) {
                        for (size_t i = 0; i < resp->n_request_headers; ++i) {
                            CCloudClientFileDownloadResponse__HTTPHeaders* hdr = resp->request_headers[i];
                            sk_ccloud_request_header_t* sk_hdr = &result->request_headers[i];
                            sk_hdr->name = hdr->name ? strdup(hdr->name) : NULL;
                            sk_hdr->value = hdr->value ? strdup(hdr->value) : NULL;
                        }
                    }
                }
            }
            ccloud__client_file_download__response__free_unpacked(resp, NULL);
        }
    }
    pthread_mutex_lock(&ctx->mutex);
    ctx->done = true;
    pthread_cond_signal(&ctx->cond);
    pthread_mutex_unlock(&ctx->mutex);
}

static void sk_steam_cloud_handle_msg(struct sk_client_msg_handler* handler, const sk_packet_msg_t* packet_msg) {
    (void)handler;
    (void)packet_msg;
}

typedef struct sk_steam_cloud {
    struct sk_client_msg_handler base;
} sk_steam_cloud_t;

sk_steam_cloud_t* sk_steam_cloud_create(void) {
    sk_steam_cloud_t* cloud = (sk_steam_cloud_t*)calloc(1, sizeof(sk_steam_cloud_t));
    if (cloud) {
        cloud->base.handle_msg = sk_steam_cloud_handle_msg;
        cloud->base.handler_type = SK_HANDLER_STEAM_CLOUD;
    }
    return cloud;
}

void sk_steam_cloud_destroy(sk_steam_cloud_t* cloud) {
    free(cloud);
}

sk_ccloud_enumerate_user_files_response_t* sk_steam_cloud_enumerate_files(sk_steam_cloud_t* cloud, uint32_t app_id) {
    if (!cloud || !cloud->base.client) return NULL;

    sk_steam_unified_messages_t* um = (sk_steam_unified_messages_t*)sk_steam_client_get_handler(cloud->base.client, SK_HANDLER_STEAM_UNIFIED_MESSAGES);
    if (!um) return NULL;

    sk_unified_service_t* svc = sk_steam_unified_messages_create_service(um, "Cloud");
    if (!svc) return NULL;

    CCloudEnumerateUserFilesRequest req = CCLOUD__ENUMERATE_USER_FILES__REQUEST__INIT;
    req.appid = app_id;
    req.has_appid = true;
    req.extended_details = true;
    req.has_extended_details = true;
    req.count = 100;
    req.has_count = true;
    req.start_index = 0;
    req.has_start_index = true;

    size_t packed_size = ccloud__enumerate_user_files__request__get_packed_size(&req);
    uint8_t* packed_buf = (uint8_t*)malloc(packed_size);
    if (!packed_buf) {
        sk_steam_unified_messages_remove_service(um, "Cloud");
        return NULL;
    }
    ccloud__enumerate_user_files__request__pack(&req, packed_buf);

    sk_cloud_req_ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    pthread_mutex_init(&ctx.mutex, NULL);
    pthread_cond_init(&ctx.cond, NULL);

    sk_steam_unified_messages_send_request(um, svc, "EnumerateUserFiles", packed_buf, packed_size, 0,
        sk_cloud_enumerate_response_cb, &ctx);

    free(packed_buf);

    pthread_mutex_lock(&ctx.mutex);
    while (!ctx.done) {
        pthread_cond_wait(&ctx.cond, &ctx.mutex);
    }
    pthread_mutex_unlock(&ctx.mutex);

    pthread_mutex_destroy(&ctx.mutex);
    pthread_cond_destroy(&ctx.cond);

    sk_steam_unified_messages_remove_service(um, "Cloud");

    if (ctx.enumerate_result) {
        sk_debug_log_info("SteamCloud", "EnumerateUserFiles returned %u files", ctx.enumerate_result->num_files);
    } else {
        sk_debug_log_warn("SteamCloud", "EnumerateUserFiles timed out or failed");
    }
    return ctx.enumerate_result;
}

void sk_steam_cloud_enumerate_files_destroy(sk_ccloud_enumerate_user_files_response_t* response) {
    if (!response) return;
    free(response->files);
    free(response);
}

sk_ugc_details_callback_t* sk_steam_cloud_request_ugc_details(sk_steam_cloud_t* cloud, uint64_t ugc_id) {
    if (!cloud || !cloud->base.client) return NULL;

    sk_steam_unified_messages_t* um = (sk_steam_unified_messages_t*)sk_steam_client_get_handler(cloud->base.client, SK_HANDLER_STEAM_UNIFIED_MESSAGES);
    if (!um) return NULL;

    sk_unified_service_t* svc = sk_steam_unified_messages_create_service(um, "Cloud");
    if (!svc) return NULL;

    CCloudGetFileDetailsRequest req = CCLOUD__GET_FILE_DETAILS__REQUEST__INIT;
    req.has_ugcid = true;
    req.ugcid = ugc_id;

    size_t packed_size = ccloud__get_file_details__request__get_packed_size(&req);
    uint8_t* packed_buf = (uint8_t*)malloc(packed_size);
    if (!packed_buf) return NULL;
    ccloud__get_file_details__request__pack(&req, packed_buf);

    sk_cloud_req_ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    pthread_mutex_init(&ctx.mutex, NULL);
    pthread_cond_init(&ctx.cond, NULL);

    sk_steam_unified_messages_send_request(um, svc, "GetFileDetails", packed_buf, packed_size, 0,
        sk_cloud_response_cb, &ctx);

    free(packed_buf);

    pthread_mutex_lock(&ctx.mutex);
    while (!ctx.done) {
        pthread_cond_wait(&ctx.cond, &ctx.mutex);
    }
    pthread_mutex_unlock(&ctx.mutex);

    pthread_mutex_destroy(&ctx.mutex);
    pthread_cond_destroy(&ctx.cond);

    if (!ctx.result) {
        sk_debug_log_warn("SteamCloud", "GetFileDetails timed out or failed");
    }
    return ctx.result;
}

sk_ccloud_client_file_download_response_t* sk_steam_cloud_client_file_download(sk_steam_cloud_t* cloud, uint32_t app_id, const char* filename) {
    if (!cloud || !cloud->base.client || !filename) return NULL;

    sk_steam_unified_messages_t* um = (sk_steam_unified_messages_t*)sk_steam_client_get_handler(cloud->base.client, SK_HANDLER_STEAM_UNIFIED_MESSAGES);
    if (!um) return NULL;

    sk_unified_service_t* svc = sk_steam_unified_messages_create_service(um, "Cloud");
    if (!svc) return NULL;

    CCloudClientFileDownloadRequest req = CCLOUD__CLIENT_FILE_DOWNLOAD__REQUEST__INIT;
    req.appid = app_id;
    req.has_appid = true;
    req.filename = (char*)filename;

    size_t packed_size = ccloud__client_file_download__request__get_packed_size(&req);
    uint8_t* packed_buf = (uint8_t*)malloc(packed_size);
    if (!packed_buf) {
        sk_steam_unified_messages_remove_service(um, "Cloud");
        return NULL;
    }
    ccloud__client_file_download__request__pack(&req, packed_buf);

    sk_cloud_req_ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    pthread_mutex_init(&ctx.mutex, NULL);
    pthread_cond_init(&ctx.cond, NULL);

    sk_steam_unified_messages_send_request(um, svc, "ClientFileDownload", packed_buf, packed_size, app_id,
        sk_cloud_file_download_response_cb, &ctx);

    free(packed_buf);

    pthread_mutex_lock(&ctx.mutex);
    while (!ctx.done) {
        pthread_cond_wait(&ctx.cond, &ctx.mutex);
    }
    pthread_mutex_unlock(&ctx.mutex);

    pthread_mutex_destroy(&ctx.mutex);
    pthread_cond_destroy(&ctx.cond);

    if (ctx.file_download_result) {
        sk_debug_log_info("SteamCloud", "ClientFileDownload returned file size %u", ctx.file_download_result->file_size);
        free(ctx.file_download_result->url_host);
        free(ctx.file_download_result->url_path);
        if (ctx.file_download_result->request_headers) {
            for (uint32_t i = 0; i < ctx.file_download_result->num_request_headers; ++i) {
                free(ctx.file_download_result->request_headers[i].name);
                free(ctx.file_download_result->request_headers[i].value);
            }
            free(ctx.file_download_result->request_headers);
        }
        free(ctx.file_download_result);
    } else {
        sk_debug_log_warn("SteamCloud", "ClientFileDownload timed out or failed");
    }
    sk_steam_unified_messages_remove_service(um, "Cloud");
    return ctx.file_download_result;
}
