#include "steamkit/steam/handlers/steam_networking.h"
#include "steamkit/steam/handlers/client_msg_handler.h"
#include <stdlib.h>

static void sk_steam_net_handle_msg(struct sk_client_msg_handler* handler, const sk_packet_msg_t* packet_msg) {
    (void)handler;
    (void)packet_msg;
}

typedef struct sk_steam_networking {
    struct sk_client_msg_handler base;
} sk_steam_networking_t;

sk_steam_networking_t* sk_steam_networking_create(void) {
    sk_steam_networking_t* networking = (sk_steam_networking_t*)calloc(1, sizeof(sk_steam_networking_t));
    if (networking) {
        networking->base.handle_msg = sk_steam_net_handle_msg;
    }
    return networking;
}

void sk_steam_networking_destroy(sk_steam_networking_t* net) {
    free(net);
}
