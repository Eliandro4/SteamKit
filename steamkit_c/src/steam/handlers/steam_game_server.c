#include "steamkit/steam/handlers/steam_game_server.h"
#include "steamkit/steam/handlers/client_msg_handler.h"
#include <stdlib.h>

static void sk_steam_gs_handle_msg(struct sk_client_msg_handler* handler, const sk_packet_msg_t* packet_msg) {
    (void)handler;
    (void)packet_msg;
}

typedef struct sk_steam_game_server {
    struct sk_client_msg_handler base;
} sk_steam_game_server_t;

sk_steam_game_server_t* sk_steam_game_server_create(void) {
    sk_steam_game_server_t* server = (sk_steam_game_server_t*)calloc(1, sizeof(sk_steam_game_server_t));
    if (server) {
        server->base.handle_msg = sk_steam_gs_handle_msg;
    }
    return server;
}

void sk_steam_game_server_destroy(sk_steam_game_server_t* server) {
    free(server);
}
