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

typedef enum sk_eresult {
    SK_ERESULT_OK = 1,
    SK_ERESULT_FAIL = 2,
    SK_ERESULT_INVALID_PASSWORD = 5,
    SK_ERESULT_INVALID_LOGIN_AUTH_CODE = 65,
    SK_ERESULT_TWO_FACTOR_CODE_MISMATCH = 88,
    SK_ERESULT_RATE_LIMIT_EXCEEDED = 89,
    SK_ERESULT_DUPLICATE_REQUEST = 29,
    SK_ERESULT_EXPIRED = 91,
    SK_ERESULT_FILE_NOT_FOUND = 105
} sk_eresult_t;

// Guard-code submit results
typedef enum sk_auth_guard_status {
    SK_AUTH_GUARD_OK = 0,
    SK_AUTH_GUARD_WRONG_CODE = 1,
    SK_AUTH_GUARD_FATAL = 2
} sk_auth_guard_status_t;

// Authenticator interface for Steam Guard codes
typedef struct sk_authenticator {
    const char* (*get_device_code)(void* user_data, bool previous_code_was_incorrect);
    const char* (*get_email_code)(void* user_data, const char* email, bool previous_code_was_incorrect);
    bool (*accept_device_confirmation)(void* user_data);
    const char* (*get_code)(void* user_data);
    void* user_data;
} sk_authenticator_t;

inline static const char* sk_authenticator_get_device_code(const sk_authenticator_t* auth, bool prev_incorrect) {
    return auth && auth->get_device_code ? auth->get_device_code(auth->user_data, prev_incorrect) : NULL;
}

inline static const char* sk_authenticator_get_email_code(const sk_authenticator_t* auth, const char* email, bool prev_incorrect) {
    return auth && auth->get_email_code ? auth->get_email_code(auth->user_data, email, prev_incorrect) : NULL;
}

inline static bool sk_authenticator_accept_device_confirmation(const sk_authenticator_t* auth) {
    return auth && auth->accept_device_confirmation ? auth->accept_device_confirmation(auth->user_data) : false;
}

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
     char* guard_data;
 } sk_auth_session_details_t;

// Authentication poll result
typedef struct sk_auth_poll_result {
    char* account_name;
    char* refresh_token;
    char* access_token;
    char* new_guard_data;
} sk_auth_poll_result_t;

// Steam Guard confirmation types
typedef enum sk_auth_session_guard_type {
    SK_AUTH_SESSION_GUARD_TYPE_UNKNOWN = 0,
    SK_AUTH_SESSION_GUARD_TYPE_NONE = 1,
    SK_AUTH_SESSION_GUARD_TYPE_EMAIL_CODE = 2,
    SK_AUTH_SESSION_GUARD_TYPE_DEVICE_CODE = 3,
    SK_AUTH_SESSION_GUARD_TYPE_DEVICE_CONFIRMATION = 4,
    SK_AUTH_SESSION_GUARD_TYPE_EMAIL_CONFIRMATION = 5,
    SK_AUTH_SESSION_GUARD_TYPE_MACHINE_TOKEN = 6,
    SK_AUTH_SESSION_GUARD_TYPE_LEGACY_MACHINE_AUTH = 7
} sk_auth_session_guard_type_t;

// Allowed confirmation description
typedef struct sk_auth_allowed_confirmation {
    sk_auth_session_guard_type_t confirmation_type;
    char* associated_message;
} sk_auth_allowed_confirmation_t;

// QR auth session
typedef struct sk_qr_auth_session {
    sk_steam_client_t* client;
    uint64_t client_id;
    uint8_t request_id[20];
    size_t request_id_len;
    int polling_interval_ms;
    char* challenge_url;
    sk_auth_session_details_t* details;
    sk_authenticator_t authenticator;
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
    sk_authenticator_t authenticator;
    sk_auth_allowed_confirmation_t* allowed_confirmations;
    size_t num_allowed_confirmations;
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

// Get allowed confirmation types for this session
size_t sk_credentials_auth_session_get_allowed_confirmations(const sk_credentials_auth_session_t* session, const sk_auth_allowed_confirmation_t** out_confirmations);

// Update auth session with Steam Guard code
int sk_credentials_auth_session_update_with_steam_guard_code(sk_credentials_auth_session_t* session, const char* code, sk_auth_session_guard_type_t code_type);

// Update auth session with mobile app confirmation
int sk_credentials_auth_session_update_with_mobile_confirmation(sk_credentials_auth_session_t* session);

// Poll authentication session status via unified messages
// Returns raw response body bytes (caller must free with free()), or NULL on timeout/failure.
// out_body_len receives the length of the response body if non-NULL.
uint8_t* sk_auth_poll_auth_session_status(sk_steam_client_t* client,
    uint64_t client_id, const uint8_t* request_id, size_t request_id_len,
    size_t* out_body_len);

// Fetch RSA public key for password encryption
char* sk_fetch_password_rsa_key(sk_steam_client_t* client, const char* account_name, char** out_mod, char** out_exp, uint64_t* out_timestamp);

// Result lifecycle
void sk_auth_poll_result_destroy(sk_auth_poll_result_t* result);

// Generate an access token from a refresh token
sk_auth_poll_result_t* sk_auth_generate_access_token_for_app(sk_steam_authentication_t* auth, uint64_t steam_id, const char* refresh_token, bool allow_renewal);

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
