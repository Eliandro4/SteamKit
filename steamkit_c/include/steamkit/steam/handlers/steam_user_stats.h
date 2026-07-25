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

// Mirrors SteamKit2: SteamUserStats.GetNumberOfCurrentPlayers(appId)
void sk_steam_user_stats_get_number_of_current_players(sk_steam_user_stats_t* stats, uint32_t app_id);

// Mirrors SteamKit2: SteamUserStats.FindLeaderboard(appId, name)
void sk_steam_user_stats_find_leaderboard(sk_steam_user_stats_t* stats, uint32_t app_id, const char* name);

// Mirrors SteamKit2: SteamUserStats.GetLeaderboardEntries(appId, id, rangeStart, rangeEnd, dataRequest)
void sk_steam_user_stats_get_leaderboard_entries(sk_steam_user_stats_t* stats, uint32_t app_id,
                                                   int32_t leaderboard_id, int32_t range_start,
                                                   int32_t range_end, int32_t data_request);

#ifdef __cplusplus
}
#endif

#endif // STEAMKIT_STEAM_HANDLERS_STEAM_USER_STATS_H
