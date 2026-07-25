#ifndef STEAMKIT_CLOUD_SERVICE_H
#define STEAMKIT_CLOUD_SERVICE_H

#include "steamkit/steam/steam_client.h"
#include "steamkit/base/generated/steam_msg_cloud.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sk_cloud_service sk_cloud_service_t;

sk_cloud_service_t* sk_cloud_service_create(sk_steam_client_t* client);
void sk_cloud_service_destroy(sk_cloud_service_t* service);

sk_ccloud_enumerate_user_files_response_t* sk_cloud_service_enumerate_user_files(
    sk_cloud_service_t* service,
    const sk_ccloud_enumerate_user_files_request_t* request);

sk_ccloud_client_file_download_response_t* sk_cloud_service_client_file_download(
    sk_cloud_service_t* service,
    const sk_ccloud_client_file_download_request_t* request);

#ifdef __cplusplus
}
#endif

#endif // STEAMKIT_CLOUD_SERVICE_H
