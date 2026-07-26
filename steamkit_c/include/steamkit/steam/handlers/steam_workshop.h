#ifndef STEAMKIT_STEAM_HANDLERS_STEAM_WORKSHOP_H
#define STEAMKIT_STEAM_HANDLERS_STEAM_WORKSHOP_H

#include <stdint.h>
#include <stdbool.h>
#include "steamkit/steam/handlers/client_msg_handler.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sk_steam_workshop sk_steam_workshop_t;

sk_steam_workshop_t* sk_steam_workshop_create(void);
void sk_steam_workshop_destroy(sk_steam_workshop_t* workshop);
void sk_steam_workshop_subscribe_item(sk_steam_workshop_t* workshop, uint64_t published_file_id);
void sk_steam_workshop_unsubscribe_item(sk_steam_workshop_t* workshop, uint64_t published_file_id);

#ifdef __cplusplus
}
#endif

#endif // STEAMKIT_STEAM_HANDLERS_STEAM_WORKSHOP_H
