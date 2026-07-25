#include "steamkit/steam/handlers/steam_apps.h"
#include "steamkit/steam/handlers/client_msg_handler.h"
#include "steamkit/steam/steam_client.h"
#include "steamkit/utils/debug_log.h"
#include "steamkit/utils/msg_util.h"
#include "steamkit/base/generated/steam_msg_apps.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

static void sk_steam_apps_handle_msg(struct sk_client_msg_handler* handler, const sk_packet_msg_t* packet_msg) {
    (void)handler;
    if (!packet_msg) return;

    uint32_t msg_type = sk_packet_msg_msg_type(packet_msg);
    switch (msg_type) {
        case SK_EMSG_CLIENT_APP_INFO: {
            sk_debug_log_info("SteamApps", "Received app info");
            break;
        }
        case SK_EMSG_CLIENT_PACKAGE_INFO: {
            sk_debug_log_info("SteamApps", "Received package info");
            break;
        }
        case SK_EMSG_CLIENT_NEW_APPS: {
            sk_debug_log_info("SteamApps", "Received new apps notification");
            break;
        }
        case SK_EMSG_CLIENT_APPS_COMPLETE: {
            sk_debug_log_info("SteamApps", "Apps enumeration complete");
            break;
        }
        default:
            break;
    }
}

typedef struct sk_steam_apps {
    struct sk_client_msg_handler base;
} sk_steam_apps_t;

sk_steam_apps_t* sk_steam_apps_create(void) {
    sk_steam_apps_t* apps = (sk_steam_apps_t*)calloc(1, sizeof(sk_steam_apps_t));
    if (apps) {
        apps->base.handle_msg = sk_steam_apps_handle_msg;
    }
    return apps;
}

void sk_steam_apps_destroy(sk_steam_apps_t* apps) {
    free(apps);
}

void sk_steam_apps_request_app_info(sk_steam_apps_t* apps, uint32_t app_id) {
    (void)apps;
    (void)app_id;
    sk_debug_log_info("SteamApps", "Requesting app info for: %u", app_id);
}

void sk_steam_apps_request_package_info(sk_steam_apps_t* apps, uint32_t package_id) {
    (void)apps;
    (void)package_id;
    sk_debug_log_info("SteamApps", "Requesting package info for: %u", package_id);
}

sk_app_ownership_ticket_callback_t* sk_steam_apps_get_app_ownership_ticket(sk_steam_apps_t* apps, uint32_t app_id) {
    (void)apps;
    sk_debug_log_info("SteamApps", "Requesting app ownership ticket for: %u (STUB)", app_id);
    return NULL;
}

sk_free_license_callback_t* sk_steam_apps_request_free_license(sk_steam_apps_t* apps, uint32_t app_id) {
    (void)apps;
    sk_debug_log_info("SteamApps", "Requesting free license for: %u (STUB)", app_id);
    return NULL;
}

sk_depot_key_callback_t* sk_steam_apps_get_depot_decryption_key(sk_steam_apps_t* apps, uint32_t depot_id, uint32_t app_id) {
    (void)apps;
    sk_debug_log_info("SteamApps", "Requesting depot decryption key for depot: %u, app: %u (STUB)", depot_id, app_id);
    return NULL;
}
