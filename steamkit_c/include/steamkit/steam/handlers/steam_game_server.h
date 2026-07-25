#ifndef STEAMKIT_STEAM_HANDLERS_STEAM_GAME_SERVER_H
#define STEAMKIT_STEAM_HANDLERS_STEAM_GAME_SERVER_H

#include <stdint.h>
#include <stdbool.h>
#include "steamkit/steam/handlers/client_msg_handler.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sk_steam_game_server sk_steam_game_server_t;

sk_steam_game_server_t* sk_steam_game_server_create(void);
void sk_steam_game_server_destroy(sk_steam_game_server_t* server);

#ifdef __cplusplus
}
#endif

#endif // STEAMKIT_STEAM_HANDLERS_STEAM_GAME_SERVER_H
