#ifndef STEAMKIT_STEAM_HANDLERS_STEAM_UNIFIED_MESSAGES_H
#define STEAMKIT_STEAM_HANDLERS_STEAM_UNIFIED_MESSAGES_H

#include <stdint.h>
#include <stdbool.h>
#include "steamkit/steam/handlers/client_msg_handler.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sk_steam_unified_messages sk_steam_unified_messages_t;

sk_steam_unified_messages_t* sk_steam_unified_messages_create(void);
void sk_steam_unified_messages_destroy(sk_steam_unified_messages_t* uxn);

#ifdef __cplusplus
}
#endif

#endif // STEAMKIT_STEAM_HANDLERS_STEAM_UNIFIED_MESSAGES_H
