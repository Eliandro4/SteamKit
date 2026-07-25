#include "steamkit/steam/handlers/steam_user_stats.h"
#include "steamkit/steam/handlers/client_msg_handler.h"
#include "steamkit/steam/steam_client.h"
#include "steamkit/utils/debug_log.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

static void sk_steam_stats_handle_msg(struct sk_client_msg_handler* handler, const sk_packet_msg_t* packet_msg) {
    (void)handler;
    if (!packet_msg) return;

    uint32_t msg_type = sk_packet_msg_msg_type(packet_msg);
    switch (msg_type) {
        case SK_EMSG_CLIENT_USER_STATS:
        case SK_EMSG_CLIENT_USER_STATS_RECEIVED:
        case SK_EMSG_CLIENT_USER_ACHIEVEMENTS: {
            sk_debug_log_info("SteamUserStats", "Received stats/achievements update");
            break;
        }
        default:
            break;
    }
}

typedef struct sk_steam_user_stats {
    struct sk_client_msg_handler base;
} sk_steam_user_stats_t;

sk_steam_user_stats_t* sk_steam_user_stats_create(void) {
    sk_steam_user_stats_t* stats = (sk_steam_user_stats_t*)calloc(1, sizeof(sk_steam_user_stats_t));
    if (stats) {
        stats->base.handle_msg = sk_steam_stats_handle_msg;
    }
    return stats;
}

void sk_steam_user_stats_destroy(sk_steam_user_stats_t* stats) {
    free(stats);
}

void sk_steam_user_stats_request_user_stats(sk_steam_user_stats_t* stats, uint32_t app_id) {
    (void)stats;
    (void)app_id;
}
