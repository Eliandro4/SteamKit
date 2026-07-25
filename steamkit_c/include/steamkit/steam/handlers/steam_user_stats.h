#ifndef STEAMKIT_STEAM_HANDLERS_STEAM_USER_STATS_H
#define STEAMKIT_STEAM_HANDLERS_STEAM_USER_STATS_H

#include <stdint.h>
#include <stdbool.h>
#include "steamkit/steam/handlers/client_msg_handler.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sk_steam_user_stats sk_steam_user_stats_t;

sk_steam_user_stats_t* sk_steam_user_stats_create(void);
void sk_steam_user_stats_destroy(sk_steam_user_stats_t* stats);
void sk_steam_user_stats_request_user_stats(sk_steam_user_stats_t* stats, uint32_t app_id);

#ifdef __cplusplus
}
#endif

#endif // STEAMKIT_STEAM_HANDLERS_STEAM_USER_STATS_H
