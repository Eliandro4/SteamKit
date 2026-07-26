#ifndef STEAMKIT_STEAM_CALLBACKS_H
#define STEAMKIT_STEAM_CALLBACKS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "steamkit/types/steam_id.h"
#include "steamkit/types/job_id.h"
#include "steamkit/types/key_value.h"
#include "steamkit/cdn/cdn_server.h"

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
typedef struct sk_steam_client sk_steam_client_t;

// Callback message base - mirrors C# CallbackMsg
typedef struct sk_callback_msg {
    sk_job_id_t job_id;
} sk_callback_msg_t;

// Callback lifecycle

// Connected callback
typedef struct sk_connected_callback {
    sk_callback_msg_t base;
} sk_connected_callback_t;

// Disconnected callback
typedef struct sk_disconnected_callback {
    sk_callback_msg_t base;
    bool user_initiated;
} sk_disconnected_callback_t;

// Logged on callback
typedef struct sk_logged_on_callback {
    sk_callback_msg_t base;
    int result;
} sk_logged_on_callback_t;

// Logged off callback
typedef struct sk_logged_off_callback {
    sk_callback_msg_t base;
    int result;
} sk_logged_off_callback_t;

// Session token callback
typedef struct sk_session_token_callback {
    sk_callback_msg_t base;
    uint64_t session_token;
} sk_session_token_callback_t;

// Account info callback
typedef struct sk_account_info_callback {
    sk_callback_msg_t base;
    char* persona_name;
    char* country;
    uint32_t count_authed_computers;
    uint32_t account_flags;
} sk_account_info_callback_t;

// Persona state callback
typedef struct sk_persona_state_callback {
    sk_callback_msg_t base;
    sk_steam_id_t* steam_id;
    uint32_t persona_state;
} sk_persona_state_callback_t;

// Friend relationship callback
typedef struct sk_friend_relationship_callback {
    sk_callback_msg_t base;
    sk_steam_id_t* steam_id;
    uint32_t relationship;
} sk_friend_relationship_callback_t;

// SteamApps callbacks
typedef struct sk_license_list_callback {
    sk_callback_msg_t base;
    uint32_t result;
    uint32_t* package_ids;
    uint32_t num_packages;
} sk_license_list_callback_t;

typedef struct sk_free_license_callback {
    sk_callback_msg_t base;
    uint32_t result;
    uint32_t* granted_apps;
    uint32_t num_granted_apps;
    uint32_t* granted_packages;
    uint32_t num_granted_packages;
} sk_free_license_callback_t;

typedef struct sk_app_ownership_ticket_callback {
    sk_callback_msg_t base;
    uint32_t result;
    uint32_t app_id;
    uint8_t* ticket_data;
    size_t ticket_length;
} sk_app_ownership_ticket_callback_t;

typedef struct sk_depot_key_callback {
    sk_callback_msg_t base;
    uint32_t result;
    uint32_t depot_id;
    uint8_t depot_key[32];
} sk_depot_key_callback_t;

typedef struct sk_pics_access_token_callback {
    sk_callback_msg_t base;
    uint32_t* package_denied_tokens;
    uint32_t num_package_denied;
    uint32_t* app_denied_tokens;
    uint32_t num_app_denied;
    uint32_t* package_tokens;
    uint32_t* package_token_appids;
    uint32_t num_package_tokens;
    uint32_t* app_tokens;
    uint32_t* app_token_appids;
    uint32_t num_app_tokens;
} sk_pics_access_token_callback_t;

typedef struct sk_pics_changes_callback {
    sk_callback_msg_t base;
    uint32_t since_change_number;
    uint32_t current_change_number;
    bool requires_full_update;
    bool requires_full_app_update;
    bool requires_full_package_update;
    uint32_t* package_changes;
    uint32_t* package_change_numbers;
    bool* package_needs_token;
    uint32_t num_package_changes;
    uint32_t* app_changes;
    uint32_t* app_change_numbers;
    bool* app_needs_token;
    uint32_t num_app_changes;
} sk_pics_changes_callback_t;

typedef struct sk_pics_product_info_callback {
    sk_callback_msg_t base;
    bool metadata_only;
    bool response_pending;
    uint32_t* unknown_packages;
    uint32_t num_unknown_packages;
    uint32_t* unknown_apps;
    uint32_t num_unknown_apps;
    sk_key_value_t** app_info_kv;
    uint32_t* app_info_ids;
    uint32_t num_app_info;
    sk_key_value_t** package_info_kv;
    uint32_t* package_info_ids;
    uint32_t num_package_info;
} sk_pics_product_info_callback_t;

typedef struct sk_private_beta_callback {
    sk_callback_msg_t base;
    uint32_t result;
    sk_key_value_t* depot_section;
} sk_private_beta_callback_t;

// SteamContent callbacks
typedef struct sk_cdn_server_list_callback {
    sk_callback_msg_t base;
    sk_cdn_server_t** servers;
    uint32_t num_servers;
} sk_cdn_server_list_callback_t;

typedef struct sk_manifest_request_code_callback {
    sk_callback_msg_t base;
    uint32_t result;
    uint64_t request_code;
} sk_manifest_request_code_callback_t;

typedef struct sk_cdn_auth_token_callback {
    sk_callback_msg_t base;
    uint32_t result;
    char* token;
} sk_cdn_auth_token_callback_t;

// SteamCloud callbacks
typedef struct sk_ugc_details_callback {
    sk_callback_msg_t base;
    uint64_t ugc_id;
    char* file_name;
    char* url;
    uint64_t file_size;
} sk_ugc_details_callback_t;

// GC message callback
typedef struct sk_gc_message_callback {
    sk_callback_msg_t base;
    uint32_t app_id;
    uint32_t msg_type;
    uint8_t* payload;
    size_t payload_len;
} sk_gc_message_callback_t;

// Callback lifecycle
sk_callback_msg_t* sk_callback_msg_create(void);
void sk_callback_msg_set_job_id(sk_callback_msg_t* msg, const sk_job_id_t* job_id);
const sk_job_id_t* sk_callback_msg_job_id(const sk_callback_msg_t* msg);
void sk_callback_msg_destroy(sk_callback_msg_t* msg);

sk_connected_callback_t* sk_connected_callback_create(void);
sk_disconnected_callback_t* sk_disconnected_callback_create(bool user_initiated);
sk_logged_on_callback_t* sk_logged_on_callback_create(int result);
sk_logged_off_callback_t* sk_logged_off_callback_create(int result);
sk_session_token_callback_t* sk_session_token_callback_create(uint64_t session_token);
sk_account_info_callback_t* sk_account_info_callback_create(const char* persona_name, const char* country, uint32_t count_authed_computers, uint32_t account_flags);
void sk_logged_off_callback_destroy(sk_logged_off_callback_t* cb);
void sk_session_token_callback_destroy(sk_session_token_callback_t* cb);
void sk_account_info_callback_destroy(sk_account_info_callback_t* cb);
sk_persona_state_callback_t* sk_persona_state_callback_create(const sk_steam_id_t* steam_id, uint32_t state);
void sk_persona_state_callback_destroy(sk_persona_state_callback_t* cb);
sk_friend_relationship_callback_t* sk_friend_relationship_callback_create(const sk_steam_id_t* steam_id, uint32_t relationship);
void sk_friend_relationship_callback_destroy(sk_friend_relationship_callback_t* cb);

// SteamApps callbacks
sk_license_list_callback_t* sk_license_list_callback_create(uint32_t result, const uint32_t* package_ids, uint32_t num_packages);
void sk_license_list_callback_destroy(sk_license_list_callback_t* cb);

sk_free_license_callback_t* sk_free_license_callback_create(uint32_t result,
    const uint32_t* granted_apps, uint32_t num_granted_apps,
    const uint32_t* granted_packages, uint32_t num_granted_packages);
void sk_free_license_callback_destroy(sk_free_license_callback_t* cb);

sk_app_ownership_ticket_callback_t* sk_app_ownership_ticket_callback_create(uint32_t result, uint32_t app_id, const uint8_t* ticket_data, size_t ticket_length);
void sk_app_ownership_ticket_callback_destroy(sk_app_ownership_ticket_callback_t* cb);

sk_depot_key_callback_t* sk_depot_key_callback_create(uint32_t result, uint32_t depot_id, const uint8_t* depot_key);
void sk_depot_key_callback_destroy(sk_depot_key_callback_t* cb);

sk_pics_access_token_callback_t* sk_pics_access_token_callback_create(void);
void sk_pics_access_token_callback_destroy(sk_pics_access_token_callback_t* cb);

sk_pics_changes_callback_t* sk_pics_changes_callback_create(void);
void sk_pics_changes_callback_destroy(sk_pics_changes_callback_t* cb);

sk_pics_product_info_callback_t* sk_pics_product_info_callback_create(void);
void sk_pics_product_info_callback_destroy(sk_pics_product_info_callback_t* cb);

sk_private_beta_callback_t* sk_private_beta_callback_create(uint32_t result, const sk_key_value_t* depot_section);
void sk_private_beta_callback_destroy(sk_private_beta_callback_t* cb);

// SteamContent callbacks
sk_cdn_server_list_callback_t* sk_cdn_server_list_callback_create(sk_cdn_server_t** servers, uint32_t num_servers);
void sk_cdn_server_list_callback_destroy(sk_cdn_server_list_callback_t* cb);

sk_manifest_request_code_callback_t* sk_manifest_request_code_callback_create(uint32_t result, uint64_t request_code);
void sk_manifest_request_code_callback_destroy(sk_manifest_request_code_callback_t* cb);

sk_cdn_auth_token_callback_t* sk_cdn_auth_token_callback_create(uint32_t result, const char* token);
void sk_cdn_auth_token_callback_destroy(sk_cdn_auth_token_callback_t* cb);

// SteamCloud callbacks
sk_ugc_details_callback_t* sk_ugc_details_callback_create(uint64_t ugc_id, const char* file_name, const char* url, uint64_t file_size);
void sk_ugc_details_callback_destroy(sk_ugc_details_callback_t* cb);

// GameCoordinator callbacks
sk_gc_message_callback_t* sk_gc_message_callback_create(uint32_t app_id, uint32_t msg_type, const uint8_t* payload, size_t payload_len);
void sk_gc_message_callback_destroy(sk_gc_message_callback_t* cb);

// SteamMatchmaking callbacks
typedef struct sk_lobby_matchmaking_callback {
    sk_callback_msg_t base;
    uint32_t result;
    uint64_t* lobby_steam_ids;
    uint32_t num_lobbies;
    int32_t* lobby_types;
    int32_t* distances;
} sk_lobby_matchmaking_callback_t;

sk_lobby_matchmaking_callback_t* sk_lobby_matchmaking_callback_create(uint32_t result, const uint64_t* lobby_steam_ids, uint32_t num_lobbies, const int32_t* lobby_types, const int32_t* distances);
void sk_lobby_matchmaking_callback_destroy(sk_lobby_matchmaking_callback_t* cb);

#ifdef __cplusplus
}
#endif

#endif // STEAMKIT_STEAM_CALLBACKS_H
