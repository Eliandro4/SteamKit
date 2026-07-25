#include "steamkit/steam/services/cloud_service.h"
#include "steamkit/utils/debug_log.h"
#include <stdlib.h>

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
    
    (void)service;
    (void)request;
    sk_debug_log_info("Cloud", "Enumerating user files (STUB)");
    return NULL;
}

sk_ccloud_client_file_download_response_t* sk_cloud_service_client_file_download(
    sk_cloud_service_t* service,
    const sk_ccloud_client_file_download_request_t* request) {
    
    (void)service;
    (void)request;
    sk_debug_log_info("Cloud", "Client file download request (STUB)");
    return NULL;
}
