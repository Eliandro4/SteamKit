#define _GNU_SOURCE
#include "steamkit/steam/authentication/steam_authentication.h"
#include "steamkit/steam/handlers/steam_unified_messages.h"
#include "steamkit/steam/steam_client.h"
#include "steamkit/utils/debug_log.h"
#include "steamkit/utils/crypto_helper.h"
#include "steammessages_auth.steamclient.pb-c.h"
#include "steammessages_base.pb-c.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>

static uint32_t sk_get_os_type(void) {
#if defined(__linux__)
    return -203;
#elif defined(__APPLE__)
    return -83;
#elif defined(_WIN32)
    return 5;
#else
    return -203;
#endif
}

static const char* sk_auth_guard_status_to_string(sk_auth_guard_status_t status) {
    switch (status) {
        case SK_AUTH_GUARD_OK: return "OK";
        case SK_AUTH_GUARD_WRONG_CODE: return "WRONG_CODE";
        case SK_AUTH_GUARD_FATAL: return "FATAL";
        default: return "UNKNOWN";
    }
}

static bool sk_auth_is_connected(sk_steam_client_t* client) {
    return client && sk_steam_client_is_connected(client);
}

static int sk_auth_sort_allowed_confirmations_compare(const sk_auth_allowed_confirmation_t* a, const sk_auth_allowed_confirmation_t* b) {
    if (!a || !b) return 0;
    static const sk_auth_session_guard_type_t preferred_order[] = {
        SK_AUTH_SESSION_GUARD_TYPE_NONE,
        SK_AUTH_SESSION_GUARD_TYPE_DEVICE_CONFIRMATION,
        SK_AUTH_SESSION_GUARD_TYPE_DEVICE_CODE,
        SK_AUTH_SESSION_GUARD_TYPE_EMAIL_CODE,
        SK_AUTH_SESSION_GUARD_TYPE_EMAIL_CONFIRMATION,
        SK_AUTH_SESSION_GUARD_TYPE_MACHINE_TOKEN,
        SK_AUTH_SESSION_GUARD_TYPE_LEGACY_MACHINE_AUTH,
        SK_AUTH_SESSION_GUARD_TYPE_UNKNOWN
    };
    int ia = 0, ib = 0;
    for (size_t i = 0; i < sizeof(preferred_order)/sizeof(preferred_order[0]); ++i) {
        if (a->confirmation_type == preferred_order[i]) { ia = (int)i; break; }
    }
    for (size_t i = 0; i < sizeof(preferred_order)/sizeof(preferred_order[0]); ++i) {
        if (b->confirmation_type == preferred_order[i]) { ib = (int)i; break; }
    }
    return ia - ib;
}

static char* sk_strdup(const char* s) {
    if (!s) return NULL;
    size_t len = strlen(s) + 1;
    char* dup = (char*)malloc(len);
    if (dup) memcpy(dup, s, len);
    return dup;
}

static sk_auth_allowed_confirmation_t* sk_copy_allowed_confirmation(const CAuthenticationAllowedConfirmation* src) {
    if (!src) return NULL;
    sk_auth_allowed_confirmation_t* dst = (sk_auth_allowed_confirmation_t*)malloc(sizeof(sk_auth_allowed_confirmation_t));
    if (!dst) return NULL;
    dst->confirmation_type = (sk_auth_session_guard_type_t)src->confirmation_type;
    dst->associated_message = src->associated_message ? sk_strdup(src->associated_message) : NULL;
    return dst;
}

static void sk_free_allowed_confirmations(sk_auth_allowed_confirmation_t* confirmations, size_t count) {
    if (!confirmations) return;
    for (size_t i = 0; i < count; ++i) {
        free(confirmations[i].associated_message);
    }
    free(confirmations);
}

static void sk_generate_request_id(uint8_t* request_id, size_t len) {
    if (!request_id || len == 0) return;
    for (size_t i = 0; i < len; ++i) {
        request_id[i] = (uint8_t)(rand() & 0xFF);
    }
}

typedef struct sk_auth_poll_req_ctx {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    bool done;
    uint32_t eresult;
    uint8_t* body;
    size_t body_len;
} sk_auth_poll_req_ctx_t;

static void sk_auth_poll_session_status_cb(void* user_data, const uint8_t* body, size_t body_len, uint32_t eresult) {
    sk_auth_poll_req_ctx_t* ctx = (sk_auth_poll_req_ctx_t*)user_data;
    ctx->eresult = eresult;
    if (body && body_len > 0) {
        ctx->body = (uint8_t*)malloc(body_len);
        if (ctx->body) {
            memcpy(ctx->body, body, body_len);
            ctx->body_len = body_len;
        }
    }
    pthread_mutex_lock(&ctx->mutex);
    ctx->done = true;
    pthread_cond_signal(&ctx->cond);
    pthread_mutex_unlock(&ctx->mutex);
}

static sk_auth_poll_req_ctx_t* sk_auth_poll_req_ctx_create(void) {
    sk_auth_poll_req_ctx_t* ctx = (sk_auth_poll_req_ctx_t*)calloc(1, sizeof(sk_auth_poll_req_ctx_t));
    if (ctx) {
        pthread_mutex_init(&ctx->mutex, NULL);
        pthread_cond_init(&ctx->cond, NULL);
    }
    return ctx;
}

static void sk_auth_poll_req_ctx_destroy(sk_auth_poll_req_ctx_t* ctx) {
    if (!ctx) return;
    pthread_mutex_destroy(&ctx->mutex);
    pthread_cond_destroy(&ctx->cond);
    free(ctx->body);
    free(ctx);
}

static uint8_t* sk_unified_request_sync(sk_steam_client_t* client,
    const char* service_name, const char* method_name,
    const uint8_t* request_body, size_t request_body_len,
    size_t* out_body_len, uint32_t* out_eresult, int timeout_ms, sk_emsg_t msg_type) {
    if (!client || !service_name || !method_name || !request_body) return NULL;

    sk_steam_unified_messages_t* um = sk_steam_client_get_unified_messages(client);
    if (!um) return NULL;

    sk_unified_service_t* svc = sk_steam_unified_messages_create_service(um, service_name);
    if (!svc) return NULL;

    sk_auth_poll_req_ctx_t* ctx = sk_auth_poll_req_ctx_create();
    if (!ctx) {
        sk_steam_unified_messages_remove_service(um, service_name);
        return NULL;
    }

    sk_steam_unified_messages_send_request_ex(um, svc, method_name, request_body, request_body_len, 0,
        sk_auth_poll_session_status_cb, ctx, msg_type);

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += timeout_ms / 1000;
    ts.tv_nsec += (long)(timeout_ms % 1000) * 1000000L;
    if (ts.tv_nsec >= 1000000000L) {
        ts.tv_sec += ts.tv_nsec / 1000000000L;
        ts.tv_nsec %= 1000000000L;
    }

    pthread_mutex_lock(&ctx->mutex);
    while (!ctx->done) {
        if (pthread_cond_timedwait(&ctx->cond, &ctx->mutex, &ts) != 0) {
            pthread_mutex_unlock(&ctx->mutex);
            sk_auth_poll_req_ctx_destroy(ctx);
            sk_steam_unified_messages_remove_service(um, service_name);
            return NULL;
        }
    }
    pthread_mutex_unlock(&ctx->mutex);

    uint8_t* result = ctx->body;
    if (out_body_len) *out_body_len = ctx->body_len;
    if (out_eresult) *out_eresult = ctx->eresult;
    ctx->body = NULL;
    ctx->body_len = 0;
    sk_auth_poll_req_ctx_destroy(ctx);
    sk_steam_unified_messages_remove_service(um, service_name);
    return result;
}

struct sk_steam_authentication {
    sk_steam_client_t* client;
};

sk_auth_session_details_t* sk_auth_session_details_create(const char* username, const char* password) {
    sk_auth_session_details_t* details = (sk_auth_session_details_t*)calloc(1, sizeof(sk_auth_session_details_t));
    if (details) {
        details->username = sk_strdup(username);
        details->password = sk_strdup(password);
    }
    return details;
}

void sk_auth_session_details_destroy(sk_auth_session_details_t* details) {
    if (!details) return;
    free(details->username);
    free(details->password);
    free(details->steam_id);
    free(details->access_token);
    free(details->refresh_token);
    free(details->device_friendly_name);
    free(details->guard_data);
    free(details);
}

void sk_auth_poll_result_destroy(sk_auth_poll_result_t* result) {
    if (!result) return;
    free(result->account_name);
    free(result->refresh_token);
    free(result->access_token);
    free(result->new_guard_data);
    free(result);
}

uint8_t* sk_auth_poll_auth_session_status(sk_steam_client_t* client,
    uint64_t client_id, const uint8_t* request_id, size_t request_id_len,
    size_t* out_body_len) {
    if (!client) return NULL;
    if (!sk_auth_is_connected(client)) {
        sk_debug_log_warn("Auth", "The SteamClient instance must be connected.");
        return NULL;
    }

    sk_steam_unified_messages_t* um = sk_steam_client_get_unified_messages(client);
    if (!um) return NULL;

    sk_unified_service_t* svc = sk_steam_unified_messages_create_service(um, "Authentication");
    if (!svc) return NULL;

    CAuthenticationPollAuthSessionStatusRequest req = CAUTHENTICATION__POLL_AUTH_SESSION_STATUS__REQUEST__INIT;
    req.has_client_id = true;
    req.client_id = client_id;
    req.has_request_id = true;
    req.request_id.data = (uint8_t*)request_id;
    req.request_id.len = request_id_len;

    size_t packed_size = cauthentication__poll_auth_session_status__request__get_packed_size(&req);
    uint8_t* packed_buf = (uint8_t*)malloc(packed_size);
    if (!packed_buf) {
        sk_steam_unified_messages_remove_service(um, "Authentication");
        return NULL;
    }
    cauthentication__poll_auth_session_status__request__pack(&req, packed_buf);

    size_t body_len = 0;
    uint32_t eresult = 0;
    const sk_steam_id_t* steam_id = sk_steam_client_get_steam_id(client);
    sk_emsg_t msg_type = (steam_id && sk_steam_id_to_uint64(steam_id) != 0)
        ? SK_EMSG_SERVICE_METHOD_CALL_FROM_CLIENT
        : SK_EMSG_SERVICE_METHOD_CALL_FROM_CLIENT_NON_AUTHED;
    uint8_t* body = sk_unified_request_sync(client, "Authentication", "PollAuthSessionStatus",
        packed_buf, packed_size, &body_len, &eresult, 30000, msg_type);
    free(packed_buf);
    sk_steam_unified_messages_remove_service(um, "Authentication");

    if (!body) {
        sk_debug_log_warn("Auth", "PollAuthSessionStatus timed out or failed (eresult=%u)", eresult);
        return NULL;
    }

    if (eresult != SK_ERESULT_OK) {
        sk_debug_log_warn("Auth", "PollAuthSessionStatus returned non-OK eresult=%u", eresult);
        free(body);
        return NULL;
    }

    if (out_body_len) *out_body_len = body_len;
    return body;
}

sk_steam_authentication_t* sk_steam_authentication_create(sk_steam_client_t* client) {
    sk_steam_authentication_t* auth = (sk_steam_authentication_t*)calloc(1, sizeof(sk_steam_authentication_t));
    if (auth) {
        auth->client = client;
        sk_debug_log_info("Auth", "Authentication subsystem initialized");
    }
    return auth;
}

void sk_steam_authentication_destroy(sk_steam_authentication_t* auth) {
    free(auth);
}

char* sk_fetch_password_rsa_key(sk_steam_client_t* client, const char* account_name, char** out_mod, char** out_exp, uint64_t* out_timestamp) {
    if (!client || !account_name || !out_mod || !out_exp || !out_timestamp) return NULL;
    *out_mod = NULL;
    *out_exp = NULL;
    *out_timestamp = 0;

    sk_steam_unified_messages_t* um = sk_steam_client_get_unified_messages(client);
    if (!um) return NULL;

    sk_unified_service_t* svc = sk_steam_unified_messages_create_service(um, "Authentication");
    if (!svc) return NULL;

    CAuthenticationGetPasswordRSAPublicKeyRequest req = CAUTHENTICATION__GET_PASSWORD_RSAPUBLIC_KEY__REQUEST__INIT;
    req.account_name = (char*)account_name;

    size_t packed_size = cauthentication__get_password_rsapublic_key__request__get_packed_size(&req);
    uint8_t* packed_buf = (uint8_t*)malloc(packed_size);
    if (!packed_buf) {
        sk_steam_unified_messages_remove_service(um, "Authentication");
        return NULL;
    }
    cauthentication__get_password_rsapublic_key__request__pack(&req, packed_buf);

    size_t body_len = 0;
    uint8_t* body = sk_unified_request_sync(client, "Authentication", "GetPasswordRSAPublicKey",
        packed_buf, packed_size, &body_len, NULL, 30000, SK_EMSG_SERVICE_METHOD_CALL_FROM_CLIENT_NON_AUTHED);
    free(packed_buf);
    if (!body) {
        sk_steam_unified_messages_remove_service(um, "Authentication");
        return NULL;
    }

    CAuthenticationGetPasswordRSAPublicKeyResponse* resp =
        cauthentication__get_password_rsapublic_key__response__unpack(NULL, body_len, body);
    free(body);
    if (!resp) {
        sk_steam_unified_messages_remove_service(um, "Authentication");
        return NULL;
    }

    *out_mod = resp->publickey_mod ? sk_strdup(resp->publickey_mod) : NULL;
    *out_exp = resp->publickey_exp ? sk_strdup(resp->publickey_exp) : NULL;
    if (resp->has_timestamp) {
        *out_timestamp = resp->timestamp;
    }
    char* result = NULL;
    if (*out_mod && *out_exp) {
        size_t result_len = strlen(*out_mod) + 1 + strlen(*out_exp) + 1;
        result = (char*)malloc(result_len);
        if (result) {
            snprintf(result, result_len, "%s|%s", *out_mod, *out_exp);
        }
    }

    cauthentication__get_password_rsapublic_key__response__free_unpacked(resp, NULL);
    sk_steam_unified_messages_remove_service(um, "Authentication");
    return result;
}

sk_qr_auth_session_t* sk_auth_begin_session_via_qr(sk_steam_authentication_t* auth, const sk_auth_session_details_t* details) {
    if (!auth || !details || !auth->client) return NULL;
    if (!sk_auth_is_connected(auth->client)) {
        sk_debug_log_warn("Auth", "The SteamClient instance must be connected.");
        return NULL;
    }

    sk_steam_unified_messages_t* um = sk_steam_client_get_unified_messages(auth->client);
    if (!um) return NULL;

    sk_unified_service_t* svc = sk_steam_unified_messages_create_service(um, "Authentication");
    if (!svc) return NULL;

    CAuthenticationBeginAuthSessionViaQRRequest req = CAUTHENTICATION__BEGIN_AUTH_SESSION_VIA_QR__REQUEST__INIT;
    if (details->device_friendly_name) {
        req.device_friendly_name = details->device_friendly_name;
    }
    req.website_id = "Client";

    CAuthenticationDeviceDetails* device_details = (CAuthenticationDeviceDetails*)malloc(sizeof(CAuthenticationDeviceDetails));
    if (!device_details) {
        sk_steam_unified_messages_remove_service(um, "Authentication");
        return NULL;
    }
    cauthentication__device_details__init(device_details);
    device_details->device_friendly_name = details->device_friendly_name ? sk_strdup(details->device_friendly_name) : NULL;
    device_details->has_platform_type = true;
    device_details->platform_type = EAUTH_TOKEN_PLATFORM_TYPE__k_EAuthTokenPlatformType_SteamClient;
    device_details->has_os_type = true;
    device_details->os_type = sk_get_os_type();
    req.device_details = device_details;

    size_t packed_size = cauthentication__begin_auth_session_via_qr__request__get_packed_size(&req);
    uint8_t* packed_buf = (uint8_t*)malloc(packed_size);
    if (!packed_buf) {
        free(device_details->device_friendly_name);
        free(device_details);
        sk_steam_unified_messages_remove_service(um, "Authentication");
        return NULL;
    }
    cauthentication__begin_auth_session_via_qr__request__pack(&req, packed_buf);
    free(device_details->device_friendly_name);
    free(device_details);

    size_t body_len = 0;
    uint32_t eresult = 0;
    uint8_t* body = sk_unified_request_sync(auth->client, "Authentication", "BeginAuthSessionViaQR",
        packed_buf, packed_size, &body_len, &eresult, 30000, SK_EMSG_SERVICE_METHOD_CALL_FROM_CLIENT_NON_AUTHED);
    free(packed_buf);
    if (!body) {
        sk_steam_unified_messages_remove_service(um, "Authentication");
        return NULL;
    }
    if (eresult != SK_ERESULT_OK) {
        sk_debug_log_warn("Auth", "BeginAuthSessionViaQR returned eresult=%u", eresult);
        free(body);
        sk_steam_unified_messages_remove_service(um, "Authentication");
        return NULL;
    }

    CAuthenticationBeginAuthSessionViaQRResponse* response =
        cauthentication__begin_auth_session_via_qr__response__unpack(NULL, body_len, body);
    free(body);
    if (!response || !response->has_client_id || !response->has_request_id) {
        cauthentication__begin_auth_session_via_qr__response__free_unpacked(response, NULL);
        sk_steam_unified_messages_remove_service(um, "Authentication");
        return NULL;
    }

    sk_qr_auth_session_t* session = (sk_qr_auth_session_t*)calloc(1, sizeof(sk_qr_auth_session_t));
    if (!session) {
        cauthentication__begin_auth_session_via_qr__response__free_unpacked(response, NULL);
        sk_steam_unified_messages_remove_service(um, "Authentication");
        return NULL;
    }

    session->client = auth->client;
    session->client_id = response->client_id;
    session->request_id_len = response->request_id.len;
    if (response->request_id.len > 0 && response->request_id.data) {
        memcpy(session->request_id, response->request_id.data, response->request_id.len);
    }
    if (response->has_interval) {
        session->polling_interval_ms = (int)(response->interval * 1000);
    } else {
        session->polling_interval_ms = 3000;
    }
    session->challenge_url = response->challenge_url ? sk_strdup(response->challenge_url) : NULL;
    session->details = sk_auth_session_details_create(details->username, details->password);
    if (session->details && details->device_friendly_name) {
        session->details->device_friendly_name = sk_strdup(details->device_friendly_name);
        session->details->is_persistent_session = details->is_persistent_session;
    }
    session->authenticator = details->authenticator;

    cauthentication__begin_auth_session_via_qr__response__free_unpacked(response, NULL);
    sk_steam_unified_messages_remove_service(um, "Authentication");
    sk_debug_log_info("Auth", "QR Session started with client_id=%llu", (unsigned long long)session->client_id);
    return session;
}

const char* sk_qr_auth_session_challenge_url(const sk_qr_auth_session_t* session) {
    return session ? session->challenge_url : NULL;
}

void sk_qr_auth_session_set_challenge_url_changed(sk_qr_auth_session_t* session,
    void (*cb)(const char* new_url, void* user_data), void* user_data) {
    if (!session) return;
    session->challenge_url_changed = cb;
    session->challenge_url_changed_user_data = user_data;
}

sk_auth_poll_result_t* sk_qr_auth_session_poll_wait_for_result(sk_qr_auth_session_t* session) {
    if (!session) return NULL;
    sk_debug_log_info("Auth", "Polling QR Session (client_id=%llu)", (unsigned long long)session->client_id);

    for (;;) {
        size_t body_len = 0;
        uint8_t* body = sk_auth_poll_auth_session_status(session->client, session->client_id,
            session->request_id, session->request_id_len, &body_len);
        if (!body) {
            sk_debug_log_warn("Auth", "PollAuthSessionStatus request failed or timed out");
            return NULL;
        }

        CAuthenticationPollAuthSessionStatusResponse* response =
            cauthentication__poll_auth_session_status__response__unpack(NULL, body_len, body);
        free(body);
        if (!response) {
            sk_debug_log_warn("Auth", "Failed to unpack PollAuthSessionStatus response");
            return NULL;
        }

        if (response->new_client_id != 0 && response->new_client_id != session->client_id) {
            session->client_id = response->new_client_id;
        }

        if (response->new_challenge_url && strlen(response->new_challenge_url) > 0) {
            if (!session->challenge_url || strcmp(session->challenge_url, response->new_challenge_url) != 0) {
                free(session->challenge_url);
                session->challenge_url = sk_strdup(response->new_challenge_url);
                if (session->challenge_url_changed) {
                    session->challenge_url_changed(session->challenge_url, session->challenge_url_changed_user_data);
                }
            }
        }

        if (response->refresh_token && strlen(response->refresh_token) > 0) {
            sk_auth_poll_result_t* result = (sk_auth_poll_result_t*)calloc(1, sizeof(sk_auth_poll_result_t));
            if (result) {
                result->account_name = sk_strdup(response->account_name);
                result->refresh_token = sk_strdup(response->refresh_token);
                result->access_token = sk_strdup(response->access_token);
                result->new_guard_data = sk_strdup(response->new_guard_data);
            }
            cauthentication__poll_auth_session_status__response__free_unpacked(response, NULL);
            return result;
        }

        cauthentication__poll_auth_session_status__response__free_unpacked(response, NULL);
        if (session->polling_interval_ms > 0) {
            usleep((useconds_t)session->polling_interval_ms * 1000);
        }
    }
}

void sk_qr_auth_session_destroy(sk_qr_auth_session_t* session) {
    if (!session) return;
    free(session->challenge_url);
    sk_auth_session_details_destroy(session->details);
    free(session);
}

sk_credentials_auth_session_t* sk_auth_begin_session_via_credentials(sk_steam_authentication_t* auth, const sk_auth_session_details_t* details) {
    if (!auth || !details || !auth->client) return NULL;
    if (!sk_auth_is_connected(auth->client)) {
        sk_debug_log_warn("Auth", "The SteamClient instance must be connected.");
        return NULL;
    }

    sk_steam_unified_messages_t* um = sk_steam_client_get_unified_messages(auth->client);
    if (!um) {
        sk_debug_log_warn("Auth", "BeginAuthSessionViaCredentials: no unified messages");
        return NULL;
    }

    sk_unified_service_t* svc = sk_steam_unified_messages_create_service(um, "Authentication");
    if (!svc) {
        sk_debug_log_warn("Auth", "BeginAuthSessionViaCredentials: failed to create service");
        return NULL;
    }

    CAuthenticationBeginAuthSessionViaCredentialsRequest req = CAUTHENTICATION__BEGIN_AUTH_SESSION_VIA_CREDENTIALS__REQUEST__INIT;
    req.account_name = details->username;
    if (details->password && details->password[0] && sk_crypto_is_available()) {
        char* mod = NULL;
        char* exp = NULL;
        uint64_t rsa_timestamp = 0;
        char* pub_key = sk_fetch_password_rsa_key(auth->client, details->username, &mod, &exp, &rsa_timestamp);
        if (pub_key) {
            size_t enc_len = 0;
            uint8_t* enc = sk_crypto_rsa_encrypt((const uint8_t*)details->password, strlen(details->password),
                (const uint8_t*)pub_key, strlen(pub_key), &enc_len);
            if (enc) {
                req.encrypted_password = sk_crypto_base64_encode(enc, enc_len);
                if (rsa_timestamp != 0) {
                    req.has_encryption_timestamp = true;
                    req.encryption_timestamp = rsa_timestamp;
                }
                free(enc);
            }
            free(pub_key);
        }
        free(mod);
        free(exp);
    }
    req.has_persistence = true;
    req.persistence = details->is_persistent_session ?
        ESESSION_PERSISTENCE__k_ESessionPersistence_Persistent :
        ESESSION_PERSISTENCE__k_ESessionPersistence_Ephemeral;
    req.website_id = "Client";
    if (details->guard_data) {
        req.guard_data = details->guard_data;
    }

    CAuthenticationDeviceDetails* device_details = (CAuthenticationDeviceDetails*)malloc(sizeof(CAuthenticationDeviceDetails));
    if (device_details) {
        cauthentication__device_details__init(device_details);
        device_details->device_friendly_name = details->device_friendly_name ? sk_strdup(details->device_friendly_name) : NULL;
        device_details->has_platform_type = true;
        device_details->platform_type = EAUTH_TOKEN_PLATFORM_TYPE__k_EAuthTokenPlatformType_SteamClient;
        device_details->has_os_type = true;
        device_details->os_type = sk_get_os_type();
        req.device_details = device_details;
    }

    size_t packed_size = cauthentication__begin_auth_session_via_credentials__request__get_packed_size(&req);
    uint8_t* packed_buf = (uint8_t*)malloc(packed_size);
    if (!packed_buf) {
        sk_steam_unified_messages_remove_service(um, "Authentication");
        return NULL;
    }
    cauthentication__begin_auth_session_via_credentials__request__pack(&req, packed_buf);

    size_t body_len = 0;
    uint32_t eresult = 0;
    uint8_t* body = sk_unified_request_sync(auth->client, "Authentication", "BeginAuthSessionViaCredentials",
        packed_buf, packed_size, &body_len, &eresult, 30000, SK_EMSG_SERVICE_METHOD_CALL_FROM_CLIENT_NON_AUTHED);
    free(packed_buf);
    if (device_details) {
        free(device_details->device_friendly_name);
        free(device_details);
    }
    if (!body) {
        sk_steam_unified_messages_remove_service(um, "Authentication");
        return NULL;
    }
    if (eresult != SK_ERESULT_OK) {
        sk_debug_log_warn("Auth", "BeginAuthSessionViaCredentials returned eresult=%u", eresult);
        free(body);
        sk_steam_unified_messages_remove_service(um, "Authentication");
        return NULL;
    }

    CAuthenticationBeginAuthSessionViaCredentialsResponse* response =
        cauthentication__begin_auth_session_via_credentials__response__unpack(NULL, body_len, body);
    free(body);
    if (!response || !response->has_client_id || !response->has_request_id) {
        sk_debug_log_warn("Auth", "BeginAuthSessionViaCredentials: invalid response (response=%p, has_client_id=%d, has_request_id=%d)",
            response, response ? (int)response->has_client_id : 0, response ? (int)response->has_request_id : 0);
        cauthentication__begin_auth_session_via_credentials__response__free_unpacked(response, NULL);
        sk_steam_unified_messages_remove_service(um, "Authentication");
        return NULL;
    }

    sk_credentials_auth_session_t* session = (sk_credentials_auth_session_t*)calloc(1, sizeof(sk_credentials_auth_session_t));
    if (!session) {
        cauthentication__begin_auth_session_via_credentials__response__free_unpacked(response, NULL);
        sk_steam_unified_messages_remove_service(um, "Authentication");
        return NULL;
    }

    session->client = auth->client;
    session->client_id = response->client_id;
    session->request_id_len = response->request_id.len;
    if (response->request_id.len > 0 && response->request_id.data) {
        memcpy(session->request_id, response->request_id.data, response->request_id.len);
    }
    if (response->has_interval) {
        session->polling_interval_ms = (int)(response->interval * 1000);
    } else {
        session->polling_interval_ms = 3000;
    }
    session->steam_id = response->has_steamid ? response->steamid : 0;
    session->details = sk_auth_session_details_create(details->username, details->password);
    if (session->details) {
        session->details->device_friendly_name = sk_strdup(details->device_friendly_name);
        session->details->is_persistent_session = details->is_persistent_session;
        session->details->authenticator = details->authenticator;
    }
    session->authenticator = details->authenticator;

    if (response->n_allowed_confirmations > 0 && response->allowed_confirmations) {
        session->allowed_confirmations = (sk_auth_allowed_confirmation_t*)malloc(sizeof(sk_auth_allowed_confirmation_t) * response->n_allowed_confirmations);
        if (session->allowed_confirmations) {
            for (size_t i = 0; i < response->n_allowed_confirmations; ++i) {
                sk_auth_allowed_confirmation_t* copy = sk_copy_allowed_confirmation(response->allowed_confirmations[i]);
                if (copy) {
                    session->allowed_confirmations[i] = *copy;
                    free(copy);
                } else {
                    session->allowed_confirmations[i].confirmation_type = SK_AUTH_SESSION_GUARD_TYPE_UNKNOWN;
                    session->allowed_confirmations[i].associated_message = NULL;
                }
            }
            session->num_allowed_confirmations = response->n_allowed_confirmations;
        }
    }

    if (session->num_allowed_confirmations > 1) {
        for (size_t i = 0; i < session->num_allowed_confirmations - 1; ++i) {
            for (size_t j = i + 1; j < session->num_allowed_confirmations; ++j) {
                if (sk_auth_sort_allowed_confirmations_compare(&session->allowed_confirmations[i], &session->allowed_confirmations[j]) > 0) {
                    sk_auth_allowed_confirmation_t tmp = session->allowed_confirmations[i];
                    session->allowed_confirmations[i] = session->allowed_confirmations[j];
                    session->allowed_confirmations[j] = tmp;
                }
            }
        }
    }

    cauthentication__begin_auth_session_via_credentials__response__free_unpacked(response, NULL);
    sk_steam_unified_messages_remove_service(um, "Authentication");
    sk_debug_log_info("Auth", "Credentials Session started for user: %s (client_id=%llu)",
        details->username ? details->username : "(null)", (unsigned long long)session->client_id);

    if (details->authenticator.get_code) {
        const char* code = details->authenticator.get_code(details->authenticator.user_data);
        sk_debug_log_info("Auth", "Received authenticator code: %s", code ? code : "(null)");
    }
    return session;
}

static sk_auth_guard_status_t sk_auth_session_send_steam_guard_code(sk_credentials_auth_session_t* session, const char* code, sk_auth_session_guard_type_t code_type) {
    if (!session || !code) return SK_AUTH_GUARD_FATAL;
    return (sk_auth_guard_status_t)sk_credentials_auth_session_update_with_steam_guard_code(session, code, code_type);
}

static sk_auth_session_guard_type_t sk_auth_session_get_preferred_confirmation(sk_credentials_auth_session_t* session) {
    if (!session || session->num_allowed_confirmations == 0 || !session->allowed_confirmations) {
        return SK_AUTH_SESSION_GUARD_TYPE_UNKNOWN;
    }
    return session->allowed_confirmations[0].confirmation_type;
}

sk_auth_poll_result_t* sk_credentials_auth_session_poll_wait_for_result(sk_credentials_auth_session_t* session) {
    if (!session) return NULL;
    sk_debug_log_info("Auth", "Polling Credentials Session (client_id=%llu)", (unsigned long long)session->client_id);

    if (session->num_allowed_confirmations == 0 || !session->allowed_confirmations) {
        sk_debug_log_warn("Auth", "No allowed confirmations, cannot poll");
        return NULL;
    }

    sk_auth_session_guard_type_t preferred = sk_auth_session_get_preferred_confirmation(session);
    sk_debug_log_info("Auth", "Preferred confirmation type=%d", preferred);

    if (preferred == SK_AUTH_SESSION_GUARD_TYPE_DEVICE_CONFIRMATION) {
        if (sk_authenticator_accept_device_confirmation(&session->authenticator)) {
            sk_debug_log_info("Auth", "Waiting for device confirmation in mobile app");
            for (;;) {
                size_t body_len = 0;
                uint8_t* body = sk_auth_poll_auth_session_status(session->client, session->client_id,
                    session->request_id, session->request_id_len, &body_len);
                if (!body) {
                    sk_debug_log_warn("Auth", "PollAuthSessionStatus request failed or timed out");
                    return NULL;
                }
                CAuthenticationPollAuthSessionStatusResponse* response =
                    cauthentication__poll_auth_session_status__response__unpack(NULL, body_len, body);
                free(body);
                if (!response) {
                    sk_debug_log_warn("Auth", "Failed to unpack PollAuthSessionStatus response");
                    return NULL;
                }

                if (response->new_client_id != 0 && response->new_client_id != session->client_id) {
                    session->client_id = response->new_client_id;
                }

                sk_auth_poll_result_t* result = NULL;
                if (response->refresh_token && strlen(response->refresh_token) > 0) {
                    result = (sk_auth_poll_result_t*)calloc(1, sizeof(sk_auth_poll_result_t));
                    if (result) {
                        result->account_name = sk_strdup(response->account_name);
                        result->refresh_token = sk_strdup(response->refresh_token);
                        result->access_token = sk_strdup(response->access_token);
                        result->new_guard_data = sk_strdup(response->new_guard_data);
                    }
                }
                cauthentication__poll_auth_session_status__response__free_unpacked(response, NULL);
                if (result) return result;

                if (session->polling_interval_ms > 0) {
                    usleep((useconds_t)session->polling_interval_ms * 1000);
                }
            }
        }

        for (size_t i = 1; i < session->num_allowed_confirmations; ++i) {
            sk_auth_session_guard_type_t fallback = session->allowed_confirmations[i].confirmation_type;
            if (fallback == SK_AUTH_SESSION_GUARD_TYPE_EMAIL_CODE || fallback == SK_AUTH_SESSION_GUARD_TYPE_DEVICE_CODE) {
                sk_debug_log_info("Auth", "Falling back from device confirmation to type=%d", fallback);
                preferred = fallback;
                break;
            }
        }
        if (preferred == SK_AUTH_SESSION_GUARD_TYPE_DEVICE_CONFIRMATION) {
            sk_debug_log_warn("Auth", "No fallback confirmation available after device confirmation was declined");
            return NULL;
        }
    }

    if (preferred == SK_AUTH_SESSION_GUARD_TYPE_EMAIL_CODE || preferred == SK_AUTH_SESSION_GUARD_TYPE_DEVICE_CODE) {
        const char* associated_email = NULL;
        bool previous_was_incorrect = false;

        if (preferred == SK_AUTH_SESSION_GUARD_TYPE_EMAIL_CODE) {
            for (size_t i = 0; i < session->num_allowed_confirmations; ++i) {
                if (session->allowed_confirmations[i].confirmation_type == SK_AUTH_SESSION_GUARD_TYPE_EMAIL_CODE) {
                    associated_email = session->allowed_confirmations[i].associated_message;
                    break;
                }
            }
        }

        int code_sent = 0;

        while (!code_sent) {
            const char* code = NULL;
            if (preferred == SK_AUTH_SESSION_GUARD_TYPE_EMAIL_CODE) {
                if (session->authenticator.get_email_code) {
                    code = session->authenticator.get_email_code(session->authenticator.user_data, associated_email, previous_was_incorrect);
                } else if (session->authenticator.get_code) {
                    code = session->authenticator.get_code(session->authenticator.user_data);
                }
            } else {
                if (session->authenticator.get_device_code) {
                    code = session->authenticator.get_device_code(session->authenticator.user_data, previous_was_incorrect);
                } else if (session->authenticator.get_code) {
                    code = session->authenticator.get_code(session->authenticator.user_data);
                }
            }

            if (!code || strlen(code) == 0) {
                sk_debug_log_warn("Auth", "No code provided by authenticator");
                return NULL;
            }

            int rc = sk_auth_session_send_steam_guard_code(session, code, preferred);
            if (rc == SK_AUTH_GUARD_OK) {
                code_sent = 1;
            } else if (rc == SK_AUTH_GUARD_WRONG_CODE) {
                previous_was_incorrect = true;
            } else {
                return NULL;
            }
        }

        size_t body_len = 0;
        uint8_t* body = sk_auth_poll_auth_session_status(session->client, session->client_id,
            session->request_id, session->request_id_len, &body_len);
        if (!body) return NULL;

        CAuthenticationPollAuthSessionStatusResponse* response =
            cauthentication__poll_auth_session_status__response__unpack(NULL, body_len, body);
        free(body);
        if (!response) return NULL;

        if (response->new_client_id != 0 && response->new_client_id != session->client_id) {
            session->client_id = response->new_client_id;
        }

        sk_auth_poll_result_t* result = NULL;
        if (response->refresh_token && strlen(response->refresh_token) > 0) {
            result = (sk_auth_poll_result_t*)calloc(1, sizeof(sk_auth_poll_result_t));
            if (result) {
                result->account_name = sk_strdup(response->account_name);
                result->refresh_token = sk_strdup(response->refresh_token);
                result->access_token = sk_strdup(response->access_token);
                result->new_guard_data = sk_strdup(response->new_guard_data);
            }
        }
        cauthentication__poll_auth_session_status__response__free_unpacked(response, NULL);
        return result;
    }

    if (preferred == SK_AUTH_SESSION_GUARD_TYPE_NONE) {
        size_t body_len = 0;
        uint8_t* body = sk_auth_poll_auth_session_status(session->client, session->client_id,
            session->request_id, session->request_id_len, &body_len);
        if (!body) return NULL;

        CAuthenticationPollAuthSessionStatusResponse* response =
            cauthentication__poll_auth_session_status__response__unpack(NULL, body_len, body);
        free(body);
        if (!response) return NULL;

        if (response->new_client_id != 0 && response->new_client_id != session->client_id) {
            session->client_id = response->new_client_id;
        }

        sk_auth_poll_result_t* result = NULL;
        if (response->refresh_token && strlen(response->refresh_token) > 0) {
            result = (sk_auth_poll_result_t*)calloc(1, sizeof(sk_auth_poll_result_t));
            if (result) {
                result->account_name = sk_strdup(response->account_name);
                result->refresh_token = sk_strdup(response->refresh_token);
                result->access_token = sk_strdup(response->access_token);
                result->new_guard_data = sk_strdup(response->new_guard_data);
            }
        }
        cauthentication__poll_auth_session_status__response__free_unpacked(response, NULL);
        return result;
    }

    sk_debug_log_warn("Auth", "Preferred confirmation type=%d not handled", preferred);
    return NULL;
}

void sk_credentials_auth_session_destroy(sk_credentials_auth_session_t* session) {
    if (!session) return;
    sk_free_allowed_confirmations(session->allowed_confirmations, session->num_allowed_confirmations);
    sk_auth_session_details_destroy(session->details);
    free(session);
}

size_t sk_credentials_auth_session_get_allowed_confirmations(const sk_credentials_auth_session_t* session, const sk_auth_allowed_confirmation_t** out_confirmations) {
    if (!session || !out_confirmations) return 0;
    *out_confirmations = session->allowed_confirmations;
    return session->num_allowed_confirmations;
}

int sk_credentials_auth_session_update_with_steam_guard_code(sk_credentials_auth_session_t* session, const char* code, sk_auth_session_guard_type_t code_type) {
    if (!session || !session->client || !code) return SK_AUTH_GUARD_FATAL;

    sk_steam_unified_messages_t* um = sk_steam_client_get_unified_messages(session->client);
    if (!um) return SK_AUTH_GUARD_FATAL;

    sk_unified_service_t* svc = sk_steam_unified_messages_create_service(um, "Authentication");
    if (!svc) return SK_AUTH_GUARD_FATAL;

    CAuthenticationUpdateAuthSessionWithSteamGuardCodeRequest req = CAUTHENTICATION__UPDATE_AUTH_SESSION_WITH_STEAM_GUARD_CODE__REQUEST__INIT;
    req.has_client_id = true;
    req.client_id = session->client_id;
    req.has_steamid = true;
    req.steamid = session->steam_id;
    req.code = (char*)code;
    req.has_code_type = true;
    req.code_type = (EAuthSessionGuardType)code_type;

    size_t packed_size = cauthentication__update_auth_session_with_steam_guard_code__request__get_packed_size(&req);
    uint8_t* packed_buf = (uint8_t*)malloc(packed_size);
    if (!packed_buf) {
        sk_steam_unified_messages_remove_service(um, "Authentication");
        return SK_AUTH_GUARD_FATAL;
    }
    cauthentication__update_auth_session_with_steam_guard_code__request__pack(&req, packed_buf);

    size_t body_len = 0;
    uint32_t eresult = 0;
    uint8_t* body = sk_unified_request_sync(session->client, "Authentication", "UpdateAuthSessionWithSteamGuardCode",
        packed_buf, packed_size, &body_len, &eresult, 30000, SK_EMSG_SERVICE_METHOD_CALL_FROM_CLIENT);
    free(packed_buf);
    sk_steam_unified_messages_remove_service(um, "Authentication");

    if (!body) {
        sk_debug_log_warn("Auth", "UpdateAuthSessionWithSteamGuardCode failed or timed out (eresult=%u)", eresult);
        return SK_AUTH_GUARD_FATAL;
    }

    CAuthenticationUpdateAuthSessionWithSteamGuardCodeResponse* response =
        cauthentication__update_auth_session_with_steam_guard_code__response__unpack(NULL, body_len, body);
    free(body);
    if (!response) {
        sk_debug_log_warn("Auth", "Failed to unpack UpdateAuthSessionWithSteamGuardCode response");
        return SK_AUTH_GUARD_FATAL;
    }

    if (eresult == SK_ERESULT_OK || eresult == SK_ERESULT_DUPLICATE_REQUEST) {
        cauthentication__update_auth_session_with_steam_guard_code__response__free_unpacked(response, NULL);
        return SK_AUTH_GUARD_OK;
    }
    if (eresult == SK_ERESULT_INVALID_LOGIN_AUTH_CODE || eresult == SK_ERESULT_TWO_FACTOR_CODE_MISMATCH) {
        sk_debug_log_warn("Auth", "UpdateAuthSessionWithSteamGuardCode wrong code (eresult=%u)", eresult);
        cauthentication__update_auth_session_with_steam_guard_code__response__free_unpacked(response, NULL);
        return SK_AUTH_GUARD_WRONG_CODE;
    }

    sk_debug_log_warn("Auth", "UpdateAuthSessionWithSteamGuardCode returned eresult=%u", eresult);
    cauthentication__update_auth_session_with_steam_guard_code__response__free_unpacked(response, NULL);
    return SK_AUTH_GUARD_FATAL;
}

int sk_credentials_auth_session_update_with_mobile_confirmation(sk_credentials_auth_session_t* session) {
    if (!session || !session->client) return -1;

    sk_steam_unified_messages_t* um = sk_steam_client_get_unified_messages(session->client);
    if (!um) return -1;

    sk_unified_service_t* svc = sk_steam_unified_messages_create_service(um, "Authentication");
    if (!svc) return -1;

    CAuthenticationUpdateAuthSessionWithMobileConfirmationRequest req = CAUTHENTICATION__UPDATE_AUTH_SESSION_WITH_MOBILE_CONFIRMATION__REQUEST__INIT;
    req.has_client_id = true;
    req.client_id = session->client_id;
    req.has_steamid = true;
    req.steamid = session->steam_id;
    req.has_confirm = true;
    req.confirm = true;

    size_t packed_size = cauthentication__update_auth_session_with_mobile_confirmation__request__get_packed_size(&req);
    uint8_t* packed_buf = (uint8_t*)malloc(packed_size);
    if (!packed_buf) {
        sk_steam_unified_messages_remove_service(um, "Authentication");
        return -1;
    }
    cauthentication__update_auth_session_with_mobile_confirmation__request__pack(&req, packed_buf);

    size_t body_len = 0;
    uint32_t eresult = 0;
    uint8_t* body = sk_unified_request_sync(session->client, "Authentication", "UpdateAuthSessionWithMobileConfirmation",
        packed_buf, packed_size, &body_len, &eresult, 30000, SK_EMSG_SERVICE_METHOD_CALL_FROM_CLIENT);
    free(packed_buf);
    sk_steam_unified_messages_remove_service(um, "Authentication");

    if (!body) {
        sk_debug_log_warn("Auth", "UpdateAuthSessionWithMobileConfirmation failed or timed out (eresult=%u)", eresult);
        return -1;
    }

    CAuthenticationUpdateAuthSessionWithMobileConfirmationResponse* response =
        cauthentication__update_auth_session_with_mobile_confirmation__response__unpack(NULL, body_len, body);
    free(body);
    if (!response) {
        sk_debug_log_warn("Auth", "Failed to unpack UpdateAuthSessionWithMobileConfirmation response");
        return -1;
    }

    if (eresult != 1) {
        sk_debug_log_warn("Auth", "UpdateAuthSessionWithMobileConfirmation returned eresult=%u", eresult);
        cauthentication__update_auth_session_with_mobile_confirmation__response__free_unpacked(response, NULL);
        return -1;
    }

    cauthentication__update_auth_session_with_mobile_confirmation__response__free_unpacked(response, NULL);
    return 0;
}

sk_auth_poll_result_t* sk_auth_generate_access_token_for_app(sk_steam_authentication_t* auth, uint64_t steam_id, const char* refresh_token, bool allow_renewal) {
    if (!auth || !auth->client || !refresh_token) return NULL;
    if (!sk_auth_is_connected(auth->client)) {
        sk_debug_log_warn("Auth", "The SteamClient instance must be connected.");
        return NULL;
    }

    sk_steam_unified_messages_t* um = sk_steam_client_get_unified_messages(auth->client);
    if (!um) return NULL;

    sk_unified_service_t* svc = sk_steam_unified_messages_create_service(um, "Authentication");
    if (!svc) return NULL;

    CAuthenticationAccessTokenGenerateForAppRequest req = CAUTHENTICATION__ACCESS_TOKEN__GENERATE_FOR_APP__REQUEST__INIT;
    req.refresh_token = (char*)refresh_token;
    req.has_steamid = true;
    req.steamid = steam_id;
    if (allow_renewal) {
        req.has_renewal_type = true;
        req.renewal_type = ETOKEN_RENEWAL_TYPE__k_ETokenRenewalType_Allow;
    }

    size_t packed_size = cauthentication__access_token__generate_for_app__request__get_packed_size(&req);
    uint8_t* packed_buf = (uint8_t*)malloc(packed_size);
    if (!packed_buf) {
        sk_steam_unified_messages_remove_service(um, "Authentication");
        return NULL;
    }
    cauthentication__access_token__generate_for_app__request__pack(&req, packed_buf);

    size_t body_len = 0;
    uint32_t eresult = 0;
    uint8_t* body = sk_unified_request_sync(auth->client, "Authentication", "GenerateAccessTokenForApp",
        packed_buf, packed_size, &body_len, &eresult, 30000, SK_EMSG_SERVICE_METHOD_CALL_FROM_CLIENT);
    free(packed_buf);
    sk_steam_unified_messages_remove_service(um, "Authentication");

    if (!body) {
        sk_debug_log_warn("Auth", "GenerateAccessTokenForApp timed out or failed (eresult=%u)", eresult);
        return NULL;
    }
    if (eresult != SK_ERESULT_OK) {
        sk_debug_log_warn("Auth", "GenerateAccessTokenForApp returned eresult=%u", eresult);
        free(body);
        sk_steam_unified_messages_remove_service(um, "Authentication");
        return NULL;
    }

    CAuthenticationAccessTokenGenerateForAppResponse* response =
        cauthentication__access_token__generate_for_app__response__unpack(NULL, body_len, body);
    free(body);
    if (!response) {
        sk_debug_log_warn("Auth", "Failed to unpack GenerateAccessTokenForApp response");
        return NULL;
    }

    sk_auth_poll_result_t* result = (sk_auth_poll_result_t*)calloc(1, sizeof(sk_auth_poll_result_t));
    if (result) {
        result->access_token = response->access_token ? sk_strdup(response->access_token) : NULL;
        result->refresh_token = response->refresh_token ? sk_strdup(response->refresh_token) : NULL;
    }

    cauthentication__access_token__generate_for_app__response__free_unpacked(response, NULL);
    return result;
}
