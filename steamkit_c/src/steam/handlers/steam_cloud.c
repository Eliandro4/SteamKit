#include "steamkit/steam/handlers/steam_cloud.h"
#include "steamkit/steam/handlers/client_msg_handler.h"
#include "steamkit/steam/handlers/steam_unified_messages.h"
#include "steamkit/steam/steam_client.h"
#include "steamkit/steam/callbacks.h"
#include "steamkit/utils/debug_log.h"
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

void sk_steam_cloud_enumerate_files(sk_steam_cloud_t* cloud, uint32_t app_id) {
    (void)cloud;
    (void)app_id;
    sk_debug_log_info("SteamCloud", "Enumerate files not yet implemented");
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
