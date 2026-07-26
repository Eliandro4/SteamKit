#ifndef STEAMKIT_STEAM_HANDLERS_STEAM_MATCHMAKING_H
#define STEAMKIT_STEAM_HANDLERS_STEAM_MATCHMAKING_H

#include <stdint.h>
#include <stdbool.h>
#include "steamkit/steam/handlers/client_msg_handler.h"
#include "steamkit/steam/callbacks.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sk_steam_matchmaking sk_steam_matchmaking_t;

sk_steam_matchmaking_t* sk_steam_matchmaking_create(void);
void sk_steam_matchmaking_destroy(sk_steam_matchmaking_t* mm);
sk_lobby_matchmaking_callback_t* sk_steam_matchmaking_request_lobby_list(sk_steam_matchmaking_t* mm, uint32_t app_id, uint32_t num_lobbies_requested, uint32_t cell_id);

#ifdef __cplusplus
}
#endif

#endif // STEAMKIT_STEAM_HANDLERS_STEAM_MATCHMAKING_H
