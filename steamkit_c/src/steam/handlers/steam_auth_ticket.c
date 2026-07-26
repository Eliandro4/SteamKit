#include "steamkit/steam/handlers/steam_auth_ticket.h"
#include "steamkit/steam/handlers/client_msg_handler.h"
#include <stdlib.h>

static void sk_steam_auth_handle_msg(struct sk_client_msg_handler* handler, const sk_packet_msg_t* packet_msg) {
    (void)handler;
    (void)packet_msg;
}

typedef struct sk_steam_auth_ticket {
    struct sk_client_msg_handler base;
} sk_steam_auth_ticket_t;

sk_steam_auth_ticket_t* sk_steam_auth_ticket_create(void) {
    sk_steam_auth_ticket_t* auth = (sk_steam_auth_ticket_t*)calloc(1, sizeof(sk_steam_auth_ticket_t));
    if (auth) {
        auth->base.handle_msg = sk_steam_auth_handle_msg;
        auth->base.handler_type = SK_HANDLER_STEAM_AUTH_TICKET;
    }
    return auth;
}

void sk_steam_auth_ticket_destroy(sk_steam_auth_ticket_t* auth) {
    free(auth);
}
