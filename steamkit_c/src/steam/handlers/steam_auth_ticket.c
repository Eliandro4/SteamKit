#include "steamkit/steam/handlers/steam_auth_ticket.h"
#include "steamkit/steam/handlers/client_msg_handler.h"
#include "steamkit/steam/handlers/steam_unified_messages.h"
#include "steamkit/steam/steam_client.h"
#include "steamkit/steam/callbacks.h"
#include "steamkit/utils/debug_log.h"
#include "steamkit/steam/handlers/client_msg_protobuf.h"
#include "steammessages_clientserver.pb-c.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

typedef struct sk_steam_auth_ticket {
    struct sk_client_msg_handler base;
} sk_steam_auth_ticket_t;

static void sk_steam_auth_handle_msg(struct sk_client_msg_handler* handler, const sk_packet_msg_t* packet_msg) {
    if (!handler || !packet_msg) return;
    (void)handler;
    (void)packet_msg;
    sk_debug_log_info("SteamAuthTicket", "Received message");
}

typedef struct sk_at_req_ctx {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    bool done;
    int32_t eresult;
} sk_at_req_ctx_t;

static void sk_auth_ticket_response_cb(void* user_data, const uint8_t* body, size_t body_len, uint32_t eresult) {
    sk_at_req_ctx_t* ctx = (sk_at_req_ctx_t*)user_data;
    ctx->eresult = eresult;
    pthread_mutex_lock(&ctx->mutex);
    ctx->done = true;
    pthread_cond_signal(&ctx->cond);
    pthread_mutex_unlock(&ctx->mutex);
}

sk_steam_auth_ticket_t* sk_steam_auth_ticket_create(void) {
    sk_steam_auth_ticket_t* auth = (sk_steam_auth_ticket_t*)calloc(1, sizeof(sk_steam_auth_ticket_t));
    if (auth) {
        auth->base.handle_msg = sk_steam_auth_handle_msg;
        auth->base.handler_type = SK_HANDLER_STEAM_AUTH_TICKET;
    }
    return auth;
}

void sk_steam_auth_ticket_destroy(sk_steam_auth_ticket_t* auth_ticket) {
    free(auth_ticket);
}

void sk_steam_auth_ticket_begin_session(sk_steam_auth_ticket_t* auth_ticket, uint32_t app_id) {
    if (!auth_ticket || !auth_ticket->base.client) return;

    sk_steam_unified_messages_t* um = (sk_steam_unified_messages_t*)sk_steam_client_get_handler(auth_ticket->base.client, SK_HANDLER_STEAM_UNIFIED_MESSAGES);
    if (!um) return;

    sk_unified_service_t* svc = sk_steam_unified_messages_create_service(um, "Auth");
    if (!svc) return;

    CMsgClientAuthList req = CMSG_CLIENT_AUTH_LIST__INIT;
    req.has_tokens_left = true;
    req.tokens_left = 1;
    req.has_last_request_seq = true;
    req.last_request_seq = 0;
    req.has_message_sequence = true;
    req.message_sequence = 1;

    size_t packed_size = cmsg_client_auth_list__get_packed_size(&req);
    uint8_t* packed_buf = (uint8_t*)malloc(packed_size);
    if (!packed_buf) {
        sk_steam_unified_messages_remove_service(um, "Auth");
        return;
    }
    cmsg_client_auth_list__pack(&req, packed_buf);

    sk_at_req_ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    pthread_mutex_init(&ctx.mutex, NULL);
    pthread_cond_init(&ctx.cond, NULL);

    sk_steam_unified_messages_send_request(um, svc, "BeginAuthSession", packed_buf, packed_size, app_id,
        sk_auth_ticket_response_cb, &ctx);

    free(packed_buf);

    pthread_mutex_lock(&ctx.mutex);
    while (!ctx.done) {
        pthread_cond_wait(&ctx.cond, &ctx.mutex);
    }
    pthread_mutex_unlock(&ctx.mutex);

    pthread_mutex_destroy(&ctx.mutex);
    pthread_cond_destroy(&ctx.cond);

    sk_steam_unified_messages_remove_service(um, "Auth");
    sk_debug_log_info("SteamAuthTicket", "Begin session for app_id=%u (eresult=%d)", app_id, ctx.eresult);
}