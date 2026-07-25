#ifndef STEAMKIT_STEAM_HANDLERS_STEAM_APPS_H
#define STEAMKIT_STEAM_HANDLERS_STEAM_APPS_H

#include <stdint.h>
#include <stdbool.h>
#include "steamkit/steam/handlers/client_msg_handler.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sk_steam_apps sk_steam_apps_t;

sk_steam_apps_t* sk_steam_apps_create(void);
void sk_steam_apps_destroy(sk_steam_apps_t* apps);
void sk_steam_apps_request_app_info(sk_steam_apps_t* apps, uint32_t app_id);
void sk_steam_apps_request_package_info(sk_steam_apps_t* apps, uint32_t package_id);

typedef struct sk_app_ownership_ticket_callback {
    uint32_t result; 
    uint32_t app_id;
    uint32_t ticket_length;
    uint8_t* ticket_data;
} sk_app_ownership_ticket_callback_t;

typedef struct sk_free_license_callback {
    uint32_t result; 
    uint32_t* granted_apps;
    uint32_t num_granted_apps;
    uint32_t* granted_packages;
    uint32_t num_granted_packages;
} sk_free_license_callback_t;

typedef struct sk_depot_key_callback {
    uint32_t result; 
    uint32_t depot_id;
    uint8_t depot_key[32];
} sk_depot_key_callback_t;

sk_app_ownership_ticket_callback_t* sk_steam_apps_get_app_ownership_ticket(sk_steam_apps_t* apps, uint32_t app_id);
sk_free_license_callback_t* sk_steam_apps_request_free_license(sk_steam_apps_t* apps, uint32_t app_id);
sk_depot_key_callback_t* sk_steam_apps_get_depot_decryption_key(sk_steam_apps_t* apps, uint32_t depot_id, uint32_t app_id);

#ifdef __cplusplus
}
#endif

#endif // STEAMKIT_STEAM_HANDLERS_STEAM_APPS_H
