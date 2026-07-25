#ifndef STEAMKIT_STEAM_HANDLERS_STEAM_USER_H
#define STEAMKIT_STEAM_HANDLERS_STEAM_USER_H

#include <stdint.h>
#include <stdbool.h>
#include "steamkit/steam/handlers/client_msg_handler.h"

#ifdef __cplusplus
extern "C" {
#endif

// SteamUser handler - mirrors C# SteamUser
typedef enum sk_chat_mode {
    SK_CHAT_MODE_DEFAULT = 0,
    SK_CHAT_MODE_NEW_STEAM_CHAT = 2
} sk_chat_mode_t;

typedef struct sk_steam_user sk_steam_user_t;

// Log on details
typedef struct sk_log_on_details {
    char* username;
    char* password;
    uint32_t cell_id;
    uint32_t login_id;
    char* auth_code;
    char* two_factor_code;
    bool should_remember_password;
    char* access_token;
    uint32_t account_instance;
    char* machine_name;
} sk_log_on_details_t;

sk_log_on_details_t* sk_log_on_details_create(void);
void sk_log_on_details_destroy(sk_log_on_details_t* details);

// Creates a SteamUser handler
sk_steam_user_t* sk_steam_user_create(void);

// Logs on with given details
void sk_steam_user_log_on(sk_steam_user_t* user, const sk_log_on_details_t* details);

// Logs off
void sk_steam_user_log_off(sk_steam_user_t* user);

// Destroys the handler
void sk_steam_user_destroy(sk_steam_user_t* user);

#ifdef __cplusplus
}
#endif

#endif // STEAMKIT_STEAM_HANDLERS_STEAM_USER_H
