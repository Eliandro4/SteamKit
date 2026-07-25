#ifndef STEAMKIT_STEAM_AUTHENTICATION_STEAM_AUTHENTICATION_H
#define STEAMKIT_STEAM_AUTHENTICATION_STEAM_AUTHENTICATION_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
typedef struct sk_steam_client sk_steam_client_t;

// Authenticator interface for Steam Guard codes
typedef struct sk_authenticator {
    const char* (*get_code)(void* user_data);
    void* user_data;
} sk_authenticator_t;

// Authentication session details
typedef struct sk_auth_session_details {
    char* username;
    char* password;
    char* steam_id;
    bool is_remember_password;
    char* access_token;
    char* refresh_token;
    
    char* device_friendly_name;
    bool is_persistent_session;
    sk_authenticator_t authenticator;
} sk_auth_session_details_t;

// Authentication poll result
typedef struct sk_auth_poll_result {
    char* account_name;
    char* refresh_token;
    char* new_guard_data;
} sk_auth_poll_result_t;

// Session types
typedef struct sk_qr_auth_session sk_qr_auth_session_t;
typedef struct sk_credentials_auth_session sk_credentials_auth_session_t;

// Authentication handler
typedef struct sk_steam_authentication sk_steam_authentication_t;

// Details lifecycle
sk_auth_session_details_t* sk_auth_session_details_create(const char* username, const char* password);
void sk_auth_session_details_destroy(sk_auth_session_details_t* details);

// Auth Handler lifecycle
sk_steam_authentication_t* sk_steam_authentication_create(sk_steam_client_t* client);
void sk_steam_authentication_destroy(sk_steam_authentication_t* auth);

// QR Authentication
sk_qr_auth_session_t* sk_auth_begin_session_via_qr(sk_steam_authentication_t* auth, const sk_auth_session_details_t* details);
sk_auth_poll_result_t* sk_qr_auth_session_poll_wait_for_result(sk_qr_auth_session_t* session);
void sk_qr_auth_session_destroy(sk_qr_auth_session_t* session);

// Credentials Authentication
sk_credentials_auth_session_t* sk_auth_begin_session_via_credentials(sk_steam_authentication_t* auth, const sk_auth_session_details_t* details);
sk_auth_poll_result_t* sk_credentials_auth_session_poll_wait_for_result(sk_credentials_auth_session_t* session);
void sk_credentials_auth_session_destroy(sk_credentials_auth_session_t* session);

// Result lifecycle
void sk_auth_poll_result_destroy(sk_auth_poll_result_t* result);

#ifdef __cplusplus
}
#endif

#endif // STEAMKIT_STEAM_AUTHENTICATION_STEAM_AUTHENTICATION_H
