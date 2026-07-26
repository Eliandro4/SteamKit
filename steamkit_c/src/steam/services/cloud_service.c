#include "steamkit/steam/services/cloud_service.h"
#include "steamkit/steam/handlers/steam_cloud.h"
#include "steamkit/steam/steam_client.h"
#include "steamkit/steam/handlers/client_msg_handler.h"
#include "steamkit/utils/debug_log.h"
#include "steamkit/base/generated/steam_msg_cloud.h"
#include <stdlib.h>
#include <string.h>

struct sk_cloud_service {
    sk_steam_client_t* client;
};

sk_cloud_service_t* sk_cloud_service_create(sk_steam_client_t* client) {
    sk_cloud_service_t* service = (sk_cloud_service_t*)calloc(1, sizeof(sk_cloud_service_t));
    if (service) {
        service->client = client;
    }
    return service;
}

void sk_cloud_service_destroy(sk_cloud_service_t* service) {
    free(service);
}

sk_ccloud_enumerate_user_files_response_t* sk_cloud_service_enumerate_user_files(
    sk_cloud_service_t* service,
    const sk_ccloud_enumerate_user_files_request_t* request) {

    if (!service || !service->client || !request) return NULL;

    sk_steam_cloud_t* cloud = (sk_steam_cloud_t*)sk_steam_client_get_handler(service->client, SK_HANDLER_STEAM_CLOUD);
    if (!cloud) return NULL;

    return sk_steam_cloud_enumerate_files(cloud, request->appid);
}

sk_ccloud_client_file_download_response_t* sk_cloud_service_client_file_download(
    sk_cloud_service_t* service,
    const sk_ccloud_client_file_download_request_t* request) {

    if (!service || !service->client || !request) return NULL;

    sk_steam_cloud_t* cloud = (sk_steam_cloud_t*)sk_steam_client_get_handler(service->client, SK_HANDLER_STEAM_CLOUD);
    if (!cloud) return NULL;

    return sk_steam_cloud_client_file_download(cloud, request->appid, request->filename);
}
