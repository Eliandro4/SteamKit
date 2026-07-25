#ifndef STEAMKIT_STEAM_HANDLERS_STEAM_GAME_COORDINATOR_H
#define STEAMKIT_STEAM_HANDLERS_STEAM_GAME_COORDINATOR_H

#include <stdint.h>
#include <stdbool.h>
#include "steamkit/steam/handlers/client_msg_handler.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sk_steam_game_coordinator sk_steam_game_coordinator_t;

sk_steam_game_coordinator_t* sk_steam_game_coordinator_create(void);
void sk_steam_game_coordinator_destroy(sk_steam_game_coordinator_t* gc);

#ifdef __cplusplus
}
#endif

#endif // STEAMKIT_STEAM_HANDLERS_STEAM_GAME_COORDINATOR_H
