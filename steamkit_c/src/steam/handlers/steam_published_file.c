#include "steamkit/steam/handlers/steam_published_file.h"
#include "steamkit/steam/handlers/client_msg_handler.h"
#include "steamkit/steam/handlers/steam_unified_messages.h"
#include "steamkit/steam/steam_client.h"
#include "steamkit/steam/callbacks.h"
#include "steamkit/utils/debug_log.h"
#include "steammessages_publishedfile.steamclient.pb-c.h"
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
    sk_published_file_details_t* result;
} sk_pf_req_ctx_t;

static void sk_pf_response_cb(void* user_data, const uint8_t* body, size_t body_len, uint32_t eresult) {
    (void)eresult;
    sk_pf_req_ctx_t* ctx = (sk_pf_req_ctx_t*)user_data;
    if (body && body_len > 0) {
        CPublishedFileGetDetailsResponse* resp = cpublished_file__get_details__response__unpack(NULL, body_len, body);
        if (resp && resp->publishedfiledetails && resp->n_publishedfiledetails > 0) {
            PublishedFileDetails* details = resp->publishedfiledetails[0];
            ctx->result = (sk_published_file_details_t*)calloc(1, sizeof(sk_published_file_details_t));
            if (ctx->result) {
                ctx->result->published_file_id = details->has_publishedfileid ? details->publishedfileid : 0;
                ctx->result->result = details->has_result ? details->result : 0;
                ctx->result->app_id = details->has_consumer_appid ? details->consumer_appid : 0;
                ctx->result->file_size = details->has_file_size ? details->file_size : 0;
                ctx->result->title = sk_strdup(details->title);
                ctx->result->description = sk_strdup(details->file_description);
                ctx->result->preview_url = sk_strdup(details->preview_url);
                ctx->result->file_url = sk_strdup(details->file_url);
            }
            cpublished_file__get_details__response__free_unpacked(resp, NULL);
        }
    }
    pthread_mutex_lock(&ctx->mutex);
    ctx->done = true;
    pthread_cond_signal(&ctx->cond);
    pthread_mutex_unlock(&ctx->mutex);
}

static void sk_steam_pf_handle_msg(struct sk_client_msg_handler* handler, const sk_packet_msg_t* packet_msg) {
    (void)handler;
    (void)packet_msg;
}

typedef struct sk_steam_published_file {
    struct sk_client_msg_handler base;
} sk_steam_published_file_t;

sk_steam_published_file_t* sk_steam_published_file_create(void) {
    sk_steam_published_file_t* pf = (sk_steam_published_file_t*)calloc(1, sizeof(sk_steam_published_file_t));
    if (pf) {
        pf->base.handle_msg = sk_steam_pf_handle_msg;
        pf->base.handler_type = SK_HANDLER_STEAM_PUBLISHED_FILE;
    }
    return pf;
}

void sk_steam_published_file_destroy(sk_steam_published_file_t* pf) {
    free(pf);
}

sk_published_file_details_t* sk_steam_published_file_get_details(sk_steam_published_file_t* pf, uint64_t published_file_id) {
    if (!pf || !pf->base.client) return NULL;

    sk_steam_unified_messages_t* um = (sk_steam_unified_messages_t*)sk_steam_client_get_handler(pf->base.client, SK_HANDLER_STEAM_UNIFIED_MESSAGES);
    if (!um) return NULL;

    sk_unified_service_t* svc = sk_steam_unified_messages_create_service(um, "PublishedFile");
    if (!svc) return NULL;

    CPublishedFileGetDetailsRequest req = CPUBLISHED_FILE__GET_DETAILS__REQUEST__INIT;
    req.has_appid = true;
    req.appid = 0;
    req.n_publishedfileids = 1;
    req.publishedfileids = (uint64_t*)&published_file_id;

    size_t packed_size = cpublished_file__get_details__request__get_packed_size(&req);
    uint8_t* packed_buf = (uint8_t*)malloc(packed_size);
    if (!packed_buf) return NULL;
    cpublished_file__get_details__request__pack(&req, packed_buf);

    sk_pf_req_ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    pthread_mutex_init(&ctx.mutex, NULL);
    pthread_cond_init(&ctx.cond, NULL);

    sk_steam_unified_messages_send_request(um, svc, "GetDetails", packed_buf, packed_size, 0,
        sk_pf_response_cb, &ctx);

    free(packed_buf);

    pthread_mutex_lock(&ctx.mutex);
    while (!ctx.done) {
        pthread_cond_wait(&ctx.cond, &ctx.mutex);
    }
    pthread_mutex_unlock(&ctx.mutex);

    pthread_mutex_destroy(&ctx.mutex);
    pthread_cond_destroy(&ctx.cond);

    if (!ctx.result) {
        sk_debug_log_warn("SteamPublishedFile", "GetDetails timed out or failed");
    }
    return ctx.result;
}

void sk_published_file_details_destroy(sk_published_file_details_t* details) {
    if (!details) return;
    free(details->title);
    free(details->description);
    free(details->preview_url);
    free(details->file_url);
    free(details);
}
