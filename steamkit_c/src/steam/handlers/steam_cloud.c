#include "steamkit/steam/handlers/steam_cloud.h"
#include "steamkit/steam/handlers/client_msg_handler.h"
#include "steamkit/steam/steam_client.h"
#include "steamkit/utils/debug_log.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

static void sk_steam_cloud_handle_msg(struct sk_client_msg_handler* handler, const sk_packet_msg_t* packet_msg) {
    // Steam Cloud (RemoteStorage) in modern Steam is HTTP-based (via ISteamRemoteStorage WebAPI).
    // There is no EMsg-based cloud file transfer in the current SteamKit2 SteamCloud handler.
    // This handler is intentionally empty; cloud file ops are handled via HTTP separately.
    (void)handler;
    (void)packet_msg;
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
