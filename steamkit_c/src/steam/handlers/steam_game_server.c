#include "steamkit/steam/handlers/steam_game_server.h"
#include "steamkit/steam/handlers/client_msg_handler.h"
#include "steamkit/steam/steam_client.h"
#include "steamkit/steam/callbacks.h"
#include "steamkit/utils/debug_log.h"
#include "steamkit/steam/handlers/client_msg_protobuf.h"
#include "steammessages_clientserver_login.pb-c.h"
#include "steammessages_clientserver.pb-c.h"
#include "steammessages_clientserver_gameservers.pb-c.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

typedef struct sk_steam_game_server {
    struct sk_client_msg_handler base;
} sk_steam_game_server_t;

static void sk_steam_gs_handle_msg(struct sk_client_msg_handler* handler, const sk_packet_msg_t* packet_msg) {
    if (!handler || !packet_msg) return;
    (void)handler;
    (void)packet_msg;
    sk_debug_log_info("SteamGameServer", "Received message");
}

typedef struct sk_gs_req_ctx {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    bool done;
    int32_t eresult;
} sk_gs_req_ctx_t;

static void sk_gs_logon_response_cb(void* user_data, const uint8_t* body, size_t body_len, uint32_t eresult) {
    sk_gs_req_ctx_t* ctx = (sk_gs_req_ctx_t*)user_data;
    ctx->eresult = eresult;
    pthread_mutex_lock(&ctx->mutex);
    ctx->done = true;
    pthread_cond_signal(&ctx->cond);
    pthread_mutex_unlock(&ctx->mutex);
}

sk_steam_game_server_t* sk_steam_game_server_create(void) {
    sk_steam_game_server_t* server = (sk_steam_game_server_t*)calloc(1, sizeof(sk_steam_game_server_t));
    if (server) {
        server->base.handle_msg = sk_steam_gs_handle_msg;
        server->base.handler_type = SK_HANDLER_STEAM_GAME_SERVER;
    }
    return server;
}

void sk_steam_game_server_destroy(sk_steam_game_server_t* server) {
    free(server);
}

void sk_steam_game_server_begin_session(sk_steam_game_server_t* server, uint32_t app_id) {
    if (!server || !server->base.client) return;

    sk_client_msg_protobuf_t* msg = sk_client_msg_protobuf_create(SK_EMSG_CLIENT_LOG_ON_GAME_SERVER);
    if (!msg) return;

    CMsgClientLogon logon_msg = CMSG_CLIENT_LOGON__INIT;
    logon_msg.protocol_version = 65581;
    logon_msg.has_protocol_version = true;
    logon_msg.client_os_type = 10;
    logon_msg.has_client_os_type = true;
    logon_msg.game_server_app_id = app_id;
    logon_msg.has_game_server_app_id = true;

    size_t packed_size = cmsg_client_logon__get_packed_size(&logon_msg);
    uint8_t* packed_buf = (uint8_t*)malloc(packed_size);
    if (packed_buf) {
        cmsg_client_logon__pack(&logon_msg, packed_buf);
        sk_client_msg_protobuf_set_body(msg, packed_buf, packed_size);
        free(packed_buf);
    }

    sk_packet_msg_t* pkt = sk_packet_msg_create_from_client_msg_protobuf(msg);
    if (pkt) {
        sk_steam_client_send(server->base.client, pkt);
        sk_packet_msg_destroy(pkt);
    }

    sk_client_msg_protobuf_destroy(msg);
    sk_debug_log_info("SteamGameServer", "Begin session for app_id=%u", app_id);
}