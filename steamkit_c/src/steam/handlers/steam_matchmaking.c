#include "steamkit/steam/handlers/steam_matchmaking.h"
#include "steamkit/steam/handlers/client_msg_handler.h"
#include <stdlib.h>

static void sk_steam_mm_handle_msg(struct sk_client_msg_handler* handler, const sk_packet_msg_t* packet_msg) {
    (void)handler;
    (void)packet_msg;
}

typedef struct sk_steam_matchmaking {
    struct sk_client_msg_handler base;
} sk_steam_matchmaking_t;

sk_steam_matchmaking_t* sk_steam_matchmaking_create(void) {
    sk_steam_matchmaking_t* mm = (sk_steam_matchmaking_t*)calloc(1, sizeof(sk_steam_matchmaking_t));
    if (mm) {
        mm->base.handle_msg = sk_steam_mm_handle_msg;
        mm->base.handler_type = SK_HANDLER_STEAM_MATCHMAKING;
    }
    return mm;
}

void sk_steam_matchmaking_destroy(sk_steam_matchmaking_t* mm) {
    free(mm);
}

void sk_steam_matchmaking_request_lobby_list(sk_steam_matchmaking_t* mm) {
    (void)mm;
}
