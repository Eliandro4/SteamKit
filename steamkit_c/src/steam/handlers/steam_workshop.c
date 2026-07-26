#include "steamkit/steam/handlers/steam_workshop.h"
#include "steamkit/steam/handlers/client_msg_handler.h"
#include "steamkit/steam/handlers/steam_unified_messages.h"
#include "steamkit/steam/steam_client.h"
#include "steamkit/steam/callbacks.h"
#include "steamkit/utils/debug_log.h"
#include "steamkit/steam/handlers/client_msg_protobuf.h"
#include "steammessages_clientserver.pb-c.h"
#include "steammessages_workshop.steamclient.pb-c.h"
#include "steammessages_clientserver_ucm.pb-c.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

typedef struct sk_steam_workshop {
    struct sk_client_msg_handler base;
} sk_steam_workshop_t;

static void sk_steam_workshop_handle_msg(struct sk_client_msg_handler* handler, const sk_packet_msg_t* packet_msg) {
    if (!handler || !packet_msg) return;
    sk_steam_workshop_t* workshop = (sk_steam_workshop_t*)handler;

    uint32_t msg_type = sk_packet_msg_msg_type(packet_msg);
    switch (msg_type) {
        case SK_EMSG_CLIENT_TO_GC:
            sk_debug_log_info("SteamWorkshop", "Received client to GC message");
            break;
        case SK_EMSG_CLIENT_FROM_GC:
            sk_debug_log_info("SteamWorkshop", "Received client from GC message");
            break;
        default:
            sk_debug_log_debug("SteamWorkshop", "Unhandled message type: %u", msg_type);
            break;
    }
}

typedef struct sk_workshop_req_ctx {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    bool done;
    int32_t eresult;
} sk_workshop_req_ctx_t;

static void sk_workshop_response_cb(void* user_data, const uint8_t* body, size_t body_len, uint32_t eresult) {
    sk_workshop_req_ctx_t* ctx = (sk_workshop_req_ctx_t*)user_data;
    ctx->eresult = eresult;
    if (body && body_len > 0) {
        CMsgClientUCMSetUserPublishedFileActionResponse* resp =
            cmsg_client_ucmset_user_published_file_action_response__unpack(NULL, body_len, body);
        if (resp) {
            ctx->eresult = resp->has_eresult ? resp->eresult : eresult;
            cmsg_client_ucmset_user_published_file_action_response__free_unpacked(resp, NULL);
        }
    }
    pthread_mutex_lock(&ctx->mutex);
    ctx->done = true;
    pthread_cond_signal(&ctx->cond);
    pthread_mutex_unlock(&ctx->mutex);
}

sk_steam_workshop_t* sk_steam_workshop_create(void) {
    sk_steam_workshop_t* workshop = (sk_steam_workshop_t*)calloc(1, sizeof(sk_steam_workshop_t));
    if (workshop) {
        workshop->base.handle_msg = sk_steam_workshop_handle_msg;
        workshop->base.handler_type = SK_HANDLER_STEAM_WORKSHOP;
    }
    return workshop;
}

void sk_steam_workshop_destroy(sk_steam_workshop_t* workshop) {
    free(workshop);
}

void sk_steam_workshop_subscribe_item(sk_steam_workshop_t* workshop, uint64_t published_file_id) {
    if (!workshop || !workshop->base.client) return;

    sk_steam_unified_messages_t* um = (sk_steam_unified_messages_t*)sk_steam_client_get_handler(workshop->base.client, SK_HANDLER_STEAM_UNIFIED_MESSAGES);
    if (!um) return;

    sk_unified_service_t* svc = sk_steam_unified_messages_create_service(um, "PublishedFile");
    if (!svc) return;

    CMsgClientUCMSetUserPublishedFileAction req = CMSG_CLIENT_UCMSET_USER_PUBLISHED_FILE_ACTION__INIT;
    req.published_file_id = published_file_id;
    req.has_published_file_id = true;
    req.app_id = 0;
    req.has_app_id = true;
    req.action = 2;
    req.has_action = true;

    size_t packed_size = cmsg_client_ucmset_user_published_file_action__get_packed_size(&req);
    uint8_t* packed_buf = (uint8_t*)malloc(packed_size);
    if (!packed_buf) {
        sk_steam_unified_messages_remove_service(um, "PublishedFile");
        return;
    }
    cmsg_client_ucmset_user_published_file_action__pack(&req, packed_buf);

    sk_workshop_req_ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    pthread_mutex_init(&ctx.mutex, NULL);
    pthread_cond_init(&ctx.cond, NULL);

    sk_steam_unified_messages_send_request(um, svc, "SetUserPublishedFileAction", packed_buf, packed_size, 0,
        sk_workshop_response_cb, &ctx);

    free(packed_buf);

    pthread_mutex_lock(&ctx.mutex);
    while (!ctx.done) {
        pthread_cond_wait(&ctx.cond, &ctx.mutex);
    }
    pthread_mutex_unlock(&ctx.mutex);

    pthread_mutex_destroy(&ctx.mutex);
    pthread_cond_destroy(&ctx.cond);

    sk_steam_unified_messages_remove_service(um, "PublishedFile");
    sk_debug_log_info("SteamWorkshop", "Subscribe item %llu (eresult=%d)", (unsigned long long)published_file_id, ctx.eresult);
}

void sk_steam_workshop_unsubscribe_item(sk_steam_workshop_t* workshop, uint64_t published_file_id) {
    if (!workshop || !workshop->base.client) return;

    sk_steam_unified_messages_t* um = (sk_steam_unified_messages_t*)sk_steam_client_get_handler(workshop->base.client, SK_HANDLER_STEAM_UNIFIED_MESSAGES);
    if (!um) return;

    sk_unified_service_t* svc = sk_steam_unified_messages_create_service(um, "PublishedFile");
    if (!svc) return;

    CMsgClientUCMSetUserPublishedFileAction req = CMSG_CLIENT_UCMSET_USER_PUBLISHED_FILE_ACTION__INIT;
    req.published_file_id = published_file_id;
    req.has_published_file_id = true;
    req.app_id = 0;
    req.has_app_id = true;
    req.action = 3;
    req.has_action = true;

    size_t packed_size = cmsg_client_ucmset_user_published_file_action__get_packed_size(&req);
    uint8_t* packed_buf = (uint8_t*)malloc(packed_size);
    if (!packed_buf) {
        sk_steam_unified_messages_remove_service(um, "PublishedFile");
        return;
    }
    cmsg_client_ucmset_user_published_file_action__pack(&req, packed_buf);

    sk_workshop_req_ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    pthread_mutex_init(&ctx.mutex, NULL);
    pthread_cond_init(&ctx.cond, NULL);

    sk_steam_unified_messages_send_request(um, svc, "SetUserPublishedFileAction", packed_buf, packed_size, 0,
        sk_workshop_response_cb, &ctx);

    free(packed_buf);

    pthread_mutex_lock(&ctx.mutex);
    while (!ctx.done) {
        pthread_cond_wait(&ctx.cond, &ctx.mutex);
    }
    pthread_mutex_unlock(&ctx.mutex);

    pthread_mutex_destroy(&ctx.mutex);
    pthread_cond_destroy(&ctx.cond);

    sk_steam_unified_messages_remove_service(um, "PublishedFile");
    sk_debug_log_info("SteamWorkshop", "Unsubscribe item %llu (eresult=%d)", (unsigned long long)published_file_id, ctx.eresult);
}
