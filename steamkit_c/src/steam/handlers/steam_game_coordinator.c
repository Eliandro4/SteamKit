#include "steamkit/steam/handlers/steam_game_coordinator.h"
#include "steamkit/steam/handlers/client_msg_handler.h"
#include "steamkit/steam/steam_client.h"
#include "steamkit/utils/debug_log.h"
#include "steamkit/base/generated/steam_msg_gc.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

static void sk_steam_gc_handle_msg(struct sk_client_msg_handler* handler, const sk_packet_msg_t* packet_msg) {
    (void)handler;
    if (!packet_msg) return;

    uint32_t msg_type = sk_packet_msg_msg_type(packet_msg);
    switch (msg_type) {
        case SK_EMSG_GC_TO_CLIENT: {
            sk_debug_log_info("SteamGC", "Received GC message");
            break;
        }
        case SK_EMSG_GC_HELLO: {
            sk_debug_log_info("SteamGC", "Received GC hello");
            break;
        }
        case SK_EMSG_GC_WELCOME: {
            sk_debug_log_info("SteamGC", "Received GC welcome");
            break;
        }
        default:
            break;
    }
}

typedef struct sk_steam_game_coordinator {
    struct sk_client_msg_handler base;
} sk_steam_game_coordinator_t;

sk_steam_game_coordinator_t* sk_steam_game_coordinator_create(void) {
    sk_steam_game_coordinator_t* gc = (sk_steam_game_coordinator_t*)calloc(1, sizeof(sk_steam_game_coordinator_t));
    if (gc) {
        gc->base.handle_msg = sk_steam_gc_handle_msg;
    }
    return gc;
}

void sk_steam_game_coordinator_destroy(sk_steam_game_coordinator_t* gc) {
    free(gc);
}
