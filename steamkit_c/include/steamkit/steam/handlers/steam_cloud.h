#ifndef STEAMKIT_STEAM_HANDLERS_STEAM_CLOUD_H
#define STEAMKIT_STEAM_HANDLERS_STEAM_CLOUD_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "steamkit/steam/handlers/client_msg_handler.h"
#include "steamkit/steam/callbacks.h"
#include "steamkit/base/generated/steam_msg_cloud.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sk_steam_cloud sk_steam_cloud_t;

sk_steam_cloud_t* sk_steam_cloud_create(void);
void sk_steam_cloud_destroy(sk_steam_cloud_t* cloud);
sk_ccloud_enumerate_user_files_response_t* sk_steam_cloud_enumerate_files(sk_steam_cloud_t* cloud, uint32_t app_id);
void sk_steam_cloud_enumerate_files_destroy(sk_ccloud_enumerate_user_files_response_t* response);
sk_ugc_details_callback_t* sk_steam_cloud_request_ugc_details(sk_steam_cloud_t* cloud, uint64_t ugc_id);
sk_ccloud_client_file_download_response_t* sk_steam_cloud_client_file_download(sk_steam_cloud_t* cloud, uint32_t app_id, const char* filename);

#ifdef __cplusplus
}
#endif

#endif // STEAMKIT_STEAM_HANDLERS_STEAM_CLOUD_H
