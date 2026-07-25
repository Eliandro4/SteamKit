#ifndef STEAMKIT_STEAM_HANDLERS_STEAM_MASTER_SERVER_H
#define STEAMKIT_STEAM_HANDLERS_STEAM_MASTER_SERVER_H

#include <stdint.h>
#include <stdbool.h>
#include "steamkit/steam/handlers/client_msg_handler.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sk_steam_master_server sk_steam_master_server_t;

sk_steam_master_server_t* sk_steam_master_server_create(void);
void sk_steam_master_server_destroy(sk_steam_master_server_t* ms);
void sk_steam_master_server_refresh(sk_steam_master_server_t* ms, bool full, bool all);

#ifdef __cplusplus
}
#endif

#endif // STEAMKIT_STEAM_HANDLERS_STEAM_MASTER_SERVER_H
