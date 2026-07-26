#ifndef STEAMKIT_STEAM_HANDLERS_STEAM_NETWORKING_H
#define STEAMKIT_STEAM_HANDLERS_STEAM_NETWORKING_H

#include <stdint.h>
#include <stdbool.h>
#include "steamkit/steam/handlers/client_msg_handler.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sk_steam_networking sk_steam_networking_t;

sk_steam_networking_t* sk_steam_networking_create(void);
void sk_steam_networking_destroy(sk_steam_networking_t* networking);
void sk_steam_networking_send_net_message(sk_steam_networking_t* net, uint32_t msg_type, const uint8_t* data, size_t len);

#ifdef __cplusplus
}
#endif

#endif // STEAMKIT_STEAM_HANDLERS_STEAM_NETWORKING_H
