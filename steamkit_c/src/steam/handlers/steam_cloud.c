#include "steamkit/steam/handlers/steam_cloud.h"
#include "steamkit/steam/handlers/client_msg_handler.h"
#include "steamkit/steam/steam_client.h"
#include "steamkit/utils/debug_log.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

static void sk_steam_cloud_handle_msg(struct sk_client_msg_handler* handler, const sk_packet_msg_t* packet_msg) {
    (void)handler;
    if (!packet_msg) return;

    uint32_t msg_type = sk_packet_msg_msg_type(packet_msg);
    switch (msg_type) {
        case SK_EMSG_CLIENT_CLOUD_FILE_DOWNLOAD:
        case SK_EMSG_CLIENT_CLOUD_FILE_UPLOAD:
        case SK_EMSG_CLIENT_CLOUD_FILE_ENUMERATE: {
            sk_debug_log_info("SteamCloud", "Received cloud file operation");
            break;
        }
        case SK_EMSG_CLIENT_CLOUD_FILE_RESPONSE: {
            sk_debug_log_info("SteamCloud", "Received cloud file response");
            break;
        }
        default:
            break;
    }
}

typedef struct sk_steam_cloud {
    struct sk_client_msg_handler base;
} sk_steam_cloud_t;

sk_steam_cloud_t* sk_steam_cloud_create(void) {
    sk_steam_cloud_t* cloud = (sk_steam_cloud_t*)calloc(1, sizeof(sk_steam_cloud_t));
    if (cloud) {
        cloud->base.handle_msg = sk_steam_cloud_handle_msg;
    }
    return cloud;
}

void sk_steam_cloud_destroy(sk_steam_cloud_t* cloud) {
    free(cloud);
}

void sk_steam_cloud_enumerate_files(sk_steam_cloud_t* cloud, uint32_t app_id) {
    (void)cloud;
    (void)app_id;
}
