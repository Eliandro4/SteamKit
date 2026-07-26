#ifndef STEAMKIT_STEAM_HANDLERS_STEAM_APPS_H
#define STEAMKIT_STEAM_HANDLERS_STEAM_APPS_H

#include <stdint.h>
#include <stdbool.h>
#include "steamkit/steam/handlers/client_msg_handler.h"
#include "steamkit/steam/callbacks.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sk_steam_apps sk_steam_apps_t;

typedef struct sk_pics_request {
    uint32_t id;
    uint64_t access_token;
    bool is_package;
} sk_pics_request_t;

sk_steam_apps_t* sk_steam_apps_create(void);
void sk_steam_apps_destroy(sk_steam_apps_t* apps);
void sk_steam_apps_request_app_info(sk_steam_apps_t* apps, uint32_t app_id);
void sk_steam_apps_request_package_info(sk_steam_apps_t* apps, uint32_t package_id);
void sk_steam_apps_request_pics_info(sk_steam_apps_t* apps, const sk_pics_request_t* requests, uint32_t num_requests);

sk_app_ownership_ticket_callback_t* sk_steam_apps_get_app_ownership_ticket(sk_steam_apps_t* apps, uint32_t app_id);
sk_free_license_callback_t* sk_steam_apps_request_free_license(sk_steam_apps_t* apps, uint32_t app_id);
sk_depot_key_callback_t* sk_steam_apps_get_depot_decryption_key(sk_steam_apps_t* apps, uint32_t depot_id, uint32_t app_id);
sk_pics_access_token_callback_t* sk_steam_apps_request_pics_access_tokens(sk_steam_apps_t* apps, const uint32_t* app_ids, uint32_t num_apps, const uint32_t* package_ids, uint32_t num_packages);

#ifdef __cplusplus
}
#endif

#endif // STEAMKIT_STEAM_HANDLERS_STEAM_APPS_H
