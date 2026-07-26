#include "steamkit/steam/handlers/steam_master_server.h"
#include "steamkit/steam/handlers/client_msg_handler.h"
#include "steamkit/steam/handlers/steam_unified_messages.h"
#include "steamkit/steam/steam_client.h"
#include "steamkit/steam/callbacks.h"
#include "steamkit/utils/debug_log.h"
#include "steamkit/steam/handlers/client_msg_protobuf.h"
#include "steammessages_clientserver.pb-c.h"
#include "steammessages_clientserver_gameservers.pb-c.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

typedef struct sk_steam_master_server {
    struct sk_client_msg_handler base;
} sk_steam_master_server_t;

static void sk_steam_ms_handle_msg(struct sk_client_msg_handler* handler, const sk_packet_msg_t* packet_msg) {
    if (!handler || !packet_msg) return;
    sk_steam_master_server_t* ms = (sk_steam_master_server_t*)handler;

    uint32_t msg_type = sk_packet_msg_msg_type(packet_msg);
    switch (msg_type) {
        case SK_EMSG_CLIENT_TO_GC:
            sk_debug_log_info("SteamMasterServer", "Received client to GC message");
            break;
        case SK_EMSG_CLIENT_FROM_GC:
            sk_debug_log_info("SteamMasterServer", "Received client from GC message");
            break;
        default:
            sk_debug_log_debug("SteamMasterServer", "Unhandled message type: %u", msg_type);
            break;
    }
}

typedef struct sk_ms_req_ctx {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    bool done;
    int32_t eresult;
} sk_ms_req_ctx_t;

static void sk_ms_query_response_cb(void* user_data, const uint8_t* body, size_t body_len, uint32_t eresult) {
    sk_ms_req_ctx_t* ctx = (sk_ms_req_ctx_t*)user_data;
    ctx->eresult = eresult;
    pthread_mutex_lock(&ctx->mutex);
    ctx->done = true;
    pthread_cond_signal(&ctx->cond);
    pthread_mutex_unlock(&ctx->mutex);
}

sk_steam_master_server_t* sk_steam_master_server_create(void) {
    sk_steam_master_server_t* ms = (sk_steam_master_server_t*)calloc(1, sizeof(sk_steam_master_server_t));
    if (ms) {
        ms->base.handle_msg = sk_steam_ms_handle_msg;
        ms->base.handler_type = SK_HANDLER_STEAM_MASTER_SERVER;
    }
    return ms;
}

void sk_steam_master_server_destroy(sk_steam_master_server_t* ms) {
    free(ms);
}

void sk_steam_master_server_refresh(sk_steam_master_server_t* ms, bool full, bool all) {
    if (!ms || !ms->base.client) return;

    sk_steam_unified_messages_t* um = (sk_steam_unified_messages_t*)sk_steam_client_get_handler(ms->base.client, SK_HANDLER_STEAM_UNIFIED_MESSAGES);
    if (!um) return;

    sk_unified_service_t* svc = sk_steam_unified_messages_create_service(um, "MasterServerDiscovery");
    if (!svc) return;

    CMsgClientGMSServerQuery req = CMSG_CLIENT_GMSSERVER_QUERY__INIT;
    req.has_app_id = true;
    req.app_id = 0;
    req.has_region_code = true;
    req.region_code = 0;
    req.has_max_servers = true;
    req.max_servers = full ? 5000 : 1000;
    req.filter_text = all ? "full" : "";

    size_t packed_size = cmsg_client_gmsserver_query__get_packed_size(&req);
    uint8_t* packed_buf = (uint8_t*)malloc(packed_size);
    if (!packed_buf) {
        sk_steam_unified_messages_remove_service(um, "MasterServerDiscovery");
        return;
    }
    cmsg_client_gmsserver_query__pack(&req, packed_buf);

    sk_ms_req_ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    pthread_mutex_init(&ctx.mutex, NULL);
    pthread_cond_init(&ctx.cond, NULL);

    sk_steam_unified_messages_send_request(um, svc, "ServerQuery", packed_buf, packed_size, 0,
        sk_ms_query_response_cb, &ctx);

    free(packed_buf);

    pthread_mutex_lock(&ctx.mutex);
    while (!ctx.done) {
        pthread_cond_wait(&ctx.cond, &ctx.mutex);
    }
    pthread_mutex_unlock(&ctx.mutex);

    pthread_mutex_destroy(&ctx.mutex);
    pthread_cond_destroy(&ctx.cond);

    sk_steam_unified_messages_remove_service(um, "MasterServerDiscovery");
    sk_debug_log_info("SteamMasterServer", "Refresh completed (eresult=%d)", ctx.eresult);
}