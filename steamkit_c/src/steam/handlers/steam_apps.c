#include "steamkit/steam/handlers/steam_apps.h"
#include "steamkit/steam/handlers/client_msg_handler.h"
#include "steamkit/steam/steam_client.h"
#include "steamkit/utils/debug_log.h"
#include "steamkit/utils/msg_util.h"
#include "steamkit/base/generated/steam_msg_apps.h"
#include "steamkit/steam/handlers/client_msg_protobuf.h"
#include "steammessages_clientserver_2.pb-c.h"
#include "steammessages_clientserver_appinfo.pb-c.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

static void sk_steam_apps_handle_msg(struct sk_client_msg_handler* handler, const sk_packet_msg_t* packet_msg) {
    (void)handler;
    if (!packet_msg) return;

    uint32_t msg_type = sk_packet_msg_msg_type(packet_msg);
    switch ((sk_emsg_t)msg_type) {
        case SK_EMSG_CLIENT_LICENSE_LIST: {
            sk_debug_log_info("SteamApps", "Received license list");
            break;
        }
        case SK_EMSG_CLIENT_REQUEST_FREE_LICENSE_RESPONSE: {
            sk_debug_log_info("SteamApps", "Received free license response");
            break;
        }
        case SK_EMSG_CLIENT_VAC_BAN_STATUS: {
            sk_debug_log_info("SteamApps", "Received VAC ban status");
            break;
        }
        case SK_EMSG_CLIENT_GET_APP_OWNERSHIP_TICKET_RESPONSE: {
            sk_debug_log_info("SteamApps", "Received app ownership ticket response");
            break;
        }
        case SK_EMSG_CLIENT_GET_DEPOT_DECRYPTION_KEY_RESPONSE: {
            sk_debug_log_info("SteamApps", "Received depot decryption key response");
            break;
        }
        case SK_EMSG_CLIENT_PICS_ACCESS_TOKEN_RESPONSE: {
            sk_debug_log_info("SteamApps", "Received PICS access token response");
            break;
        }
        case SK_EMSG_CLIENT_PICS_CHANGES_SINCE_RESPONSE: {
            sk_debug_log_info("SteamApps", "Received PICS changes since response");
            break;
        }
        case SK_EMSG_CLIENT_PICS_PRODUCT_INFO_RESPONSE: {
            sk_debug_log_info("SteamApps", "Received PICS product info response");
            break;
        }
        case SK_EMSG_CLIENT_PICS_PRIVATE_BETA_RESPONSE: {
            sk_debug_log_info("SteamApps", "Received PICS private beta response");
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
    if (!apps || !apps->base.client) return;
    
    sk_client_msg_protobuf_t* msg = sk_client_msg_protobuf_create(SK_EMSG_CLIENT_PICS_PRODUCT_INFO_REQUEST);
    if (!msg) return;
    
    // Build a PICS ProductInfo request with one app
    CMsgClientPICSProductInfoRequest req = CMSG_CLIENT_PICSPRODUCT_INFO_REQUEST__INIT;
    CMsgClientPICSProductInfoRequest__AppInfo app_info = CMSG_CLIENT_PICSPRODUCT_INFO_REQUEST__APP_INFO__INIT;
    app_info.appid = app_id;
    app_info.has_appid = true;
    app_info.only_public_obsolete = false;
    app_info.has_only_public_obsolete = true;
    
    CMsgClientPICSProductInfoRequest__AppInfo* apps_arr[1] = { &app_info };
    req.apps = apps_arr;
    req.n_apps = 1;
    req.meta_data_only = false;
    req.has_meta_data_only = true;
    
    size_t packed_size = cmsg_client_picsproduct_info_request__get_packed_size(&req);
    uint8_t* packed_buf = (uint8_t*)malloc(packed_size);
    if (packed_buf) {
        cmsg_client_picsproduct_info_request__pack(&req, packed_buf);
        sk_client_msg_protobuf_set_body(msg, packed_buf, packed_size);
        free(packed_buf);
    }
    
    sk_packet_msg_t* pkt = sk_packet_msg_create_from_client_msg_protobuf(msg);
    if (pkt) {
        sk_steam_client_send(apps->base.client, pkt);
        sk_packet_msg_destroy(pkt);
    }
    sk_client_msg_protobuf_destroy(msg);
    sk_debug_log_info("SteamApps", "Requesting app info for: %u", app_id);
}

void sk_steam_apps_request_package_info(sk_steam_apps_t* apps, uint32_t package_id) {
    if (!apps || !apps->base.client) return;
    
    sk_client_msg_protobuf_t* msg = sk_client_msg_protobuf_create(SK_EMSG_CLIENT_PICS_PRODUCT_INFO_REQUEST);
    if (!msg) return;
    
    CMsgClientPICSProductInfoRequest req = CMSG_CLIENT_PICSPRODUCT_INFO_REQUEST__INIT;
    CMsgClientPICSProductInfoRequest__PackageInfo pkg_info = CMSG_CLIENT_PICSPRODUCT_INFO_REQUEST__PACKAGE_INFO__INIT;
    pkg_info.packageid = package_id;
    pkg_info.has_packageid = true;
    
    CMsgClientPICSProductInfoRequest__PackageInfo* pkgs_arr[1] = { &pkg_info };
    req.packages = pkgs_arr;
    req.n_packages = 1;
    req.meta_data_only = false;
    req.has_meta_data_only = true;
    
    size_t packed_size = cmsg_client_picsproduct_info_request__get_packed_size(&req);
    uint8_t* packed_buf = (uint8_t*)malloc(packed_size);
    if (packed_buf) {
        cmsg_client_picsproduct_info_request__pack(&req, packed_buf);
        sk_client_msg_protobuf_set_body(msg, packed_buf, packed_size);
        free(packed_buf);
    }
    
    sk_packet_msg_t* pkt = sk_packet_msg_create_from_client_msg_protobuf(msg);
    if (pkt) {
        sk_steam_client_send(apps->base.client, pkt);
        sk_packet_msg_destroy(pkt);
    }
    sk_client_msg_protobuf_destroy(msg);
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
    if (!apps || !apps->base.client) return NULL;
    
    sk_client_msg_protobuf_t* msg = sk_client_msg_protobuf_create(SK_EMSG_CLIENT_GET_DEPOT_DECRYPTION_KEY);
    if (!msg) return NULL;
    
    CMsgClientGetDepotDecryptionKey req = CMSG_CLIENT_GET_DEPOT_DECRYPTION_KEY__INIT;
    req.depot_id = depot_id;
    req.has_depot_id = true;
    req.app_id = app_id;
    req.has_app_id = true;
    
    size_t packed_size = cmsg_client_get_depot_decryption_key__get_packed_size(&req);
    uint8_t* packed_buf = (uint8_t*)malloc(packed_size);
    if (packed_buf) {
        cmsg_client_get_depot_decryption_key__pack(&req, packed_buf);
        sk_client_msg_protobuf_set_body(msg, packed_buf, packed_size);
        free(packed_buf);
    }
    
    sk_packet_msg_t* pkt = sk_packet_msg_create_from_client_msg_protobuf(msg);
    if (pkt) {
        sk_steam_client_send(apps->base.client, pkt);
        sk_packet_msg_destroy(pkt);
    }
    sk_client_msg_protobuf_destroy(msg);
    
    sk_debug_log_info("SteamApps", "Requested depot decryption key for depot: %u, app: %u", depot_id, app_id);
    return NULL; // Stub returning NULL for callback handle for now
}
