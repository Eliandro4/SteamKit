#include "steamkit/steam/handlers/steam_unified_messages.h"
#include "steamkit/steam/handlers/client_msg_handler.h"
#include <stdlib.h>

static void sk_steam_uxn_handle_msg(struct sk_client_msg_handler* handler, const sk_packet_msg_t* packet_msg) {
    (void)handler;
    (void)packet_msg;
}

typedef struct sk_steam_unified_messages {
    struct sk_client_msg_handler base;
} sk_steam_unified_messages_t;

sk_steam_unified_messages_t* sk_steam_unified_messages_create(void) {
    sk_steam_unified_messages_t* uxn = (sk_steam_unified_messages_t*)calloc(1, sizeof(sk_steam_unified_messages_t));
    if (uxn) {
        uxn->base.handle_msg = sk_steam_uxn_handle_msg;
    }
    return uxn;
}

void sk_steam_unified_messages_destroy(sk_steam_unified_messages_t* uxn) {
    free(uxn);
}
