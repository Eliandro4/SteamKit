#ifndef STEAMKIT_STEAM_HANDLERS_STEAM_GAME_COORDINATOR_H
#define STEAMKIT_STEAM_HANDLERS_STEAM_GAME_COORDINATOR_H

#include <stdint.h>
#include <stdbool.h>
#include "steamkit/steam/handlers/client_msg_handler.h"
#include "steamkit/steam/callbacks.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sk_steam_game_coordinator sk_steam_game_coordinator_t;

sk_steam_game_coordinator_t* sk_steam_game_coordinator_create(void);
void sk_steam_game_coordinator_destroy(sk_steam_game_coordinator_t* gc);
void sk_steam_game_coordinator_send(sk_steam_game_coordinator_t* gc, uint32_t app_id,
                                     const uint8_t* payload, size_t payload_len,
                                     uint32_t gc_msg_type);

#ifdef __cplusplus
}
#endif

#endif // STEAMKIT_STEAM_HANDLERS_STEAM_GAME_COORDINATOR_H
