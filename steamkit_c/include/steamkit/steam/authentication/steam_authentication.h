#ifndef STEAMKIT_STEAM_AUTHENTICATION_STEAM_AUTHENTICATION_H
#define STEAMKIT_STEAM_AUTHENTICATION_STEAM_AUTHENTICATION_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

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
    char* access_token;
    char* new_guard_data;
} sk_auth_poll_result_t;

// QR auth session
typedef struct sk_qr_auth_session {
    sk_steam_client_t* client;
    uint64_t client_id;
    uint8_t request_id[20];
    size_t request_id_len;
    int polling_interval_ms;
    char* challenge_url;
    sk_auth_session_details_t* details;
    void (*challenge_url_changed)(const char* new_url, void* user_data);
    void* challenge_url_changed_user_data;
} sk_qr_auth_session_t;

// Credentials auth session
typedef struct sk_credentials_auth_session {
    sk_steam_client_t* client;
    uint64_t client_id;
    uint8_t request_id[20];
    size_t request_id_len;
    int polling_interval_ms;
    uint64_t steam_id;
    sk_auth_session_details_t* details;
} sk_credentials_auth_session_t;

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
const char* sk_qr_auth_session_challenge_url(const sk_qr_auth_session_t* session);
void sk_qr_auth_session_set_challenge_url_changed(sk_qr_auth_session_t* session,
    void (*cb)(const char* new_url, void* user_data), void* user_data);
sk_auth_poll_result_t* sk_qr_auth_session_poll_wait_for_result(sk_qr_auth_session_t* session);
void sk_qr_auth_session_destroy(sk_qr_auth_session_t* session);

// Credentials Authentication
sk_credentials_auth_session_t* sk_auth_begin_session_via_credentials(sk_steam_authentication_t* auth, const sk_auth_session_details_t* details);
sk_auth_poll_result_t* sk_credentials_auth_session_poll_wait_for_result(sk_credentials_auth_session_t* session);
void sk_credentials_auth_session_destroy(sk_credentials_auth_session_t* session);

// Poll authentication session status via unified messages
// Returns raw response body bytes (caller must free with free()), or NULL on timeout/failure.
// out_body_len receives the length of the response body if non-NULL.
uint8_t* sk_auth_poll_auth_session_status(sk_steam_client_t* client,
    uint64_t client_id, const uint8_t* request_id, size_t request_id_len,
    size_t* out_body_len);

// Result lifecycle
void sk_auth_poll_result_destroy(sk_auth_poll_result_t* result);

// Result field accessors
static inline const char* sk_auth_poll_result_account_name(const sk_auth_poll_result_t* result) {
    return result ? result->account_name : NULL;
}
static inline const char* sk_auth_poll_result_refresh_token(const sk_auth_poll_result_t* result) {
    return result ? result->refresh_token : NULL;
}
static inline const char* sk_auth_poll_result_access_token(const sk_auth_poll_result_t* result) {
    return result ? result->access_token : NULL;
}
static inline const char* sk_auth_poll_result_new_guard_data(const sk_auth_poll_result_t* result) {
    return result ? result->new_guard_data : NULL;
}

#ifdef __cplusplus
}
#endif

#endif // STEAMKIT_STEAM_AUTHENTICATION_STEAM_AUTHENTICATION_H
