#ifndef STEAMKIT_STEAM_HANDLERS_STEAM_CLOUD_H
#define STEAMKIT_STEAM_HANDLERS_STEAM_CLOUD_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "steamkit/steam/handlers/client_msg_handler.h"
#include "steamkit/steam/callbacks.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sk_steam_cloud sk_steam_cloud_t;

sk_steam_cloud_t* sk_steam_cloud_create(void);
void sk_steam_cloud_destroy(sk_steam_cloud_t* cloud);
void sk_steam_cloud_enumerate_files(sk_steam_cloud_t* cloud, uint32_t app_id);
sk_ugc_details_callback_t* sk_steam_cloud_request_ugc_details(sk_steam_cloud_t* cloud, uint64_t ugc_id);

#ifdef __cplusplus
}
#endif

#endif // STEAMKIT_STEAM_HANDLERS_STEAM_CLOUD_H
