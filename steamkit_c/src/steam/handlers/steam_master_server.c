#include "steamkit/steam/handlers/steam_master_server.h"
#include "steamkit/steam/handlers/client_msg_handler.h"
#include <stdlib.h>

static void sk_steam_ms_handle_msg(struct sk_client_msg_handler* handler, const sk_packet_msg_t* packet_msg) {
    (void)handler;
    (void)packet_msg;
}

typedef struct sk_steam_master_server {
    struct sk_client_msg_handler base;
} sk_steam_master_server_t;

sk_steam_master_server_t* sk_steam_master_server_create(void) {
    sk_steam_master_server_t* ms = (sk_steam_master_server_t*)calloc(1, sizeof(sk_steam_master_server_t));
    if (ms) {
        ms->base.handle_msg = sk_steam_ms_handle_msg;
    }
    return ms;
}

void sk_steam_master_server_destroy(sk_steam_master_server_t* ms) {
    free(ms);
}

void sk_steam_master_server_refresh(sk_steam_master_server_t* ms, bool full, bool all) {
    (void)ms;
    (void)full;
    (void)all;
}
