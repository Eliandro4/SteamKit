#ifndef STEAMKIT_STEAM_HANDLERS_STEAM_FRIENDS_H
#define STEAMKIT_STEAM_HANDLERS_STEAM_FRIENDS_H

#include <stdint.h>
#include <stdbool.h>
#include "steamkit/steam/handlers/client_msg_handler.h"
#include "steamkit/types/steam_id.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sk_steam_friends sk_steam_friends_t;

sk_steam_friends_t* sk_steam_friends_create(void);
void sk_steam_friends_destroy(sk_steam_friends_t* friends);
void sk_steam_friends_request_user_info(sk_steam_friends_t* friends, const sk_steam_id_t* steam_id);
void sk_steam_friends_set_persona_state(sk_steam_friends_t* friends, uint32_t persona_state);

#ifdef __cplusplus
}
#endif

#endif // STEAMKIT_STEAM_HANDLERS_STEAM_FRIENDS_H
