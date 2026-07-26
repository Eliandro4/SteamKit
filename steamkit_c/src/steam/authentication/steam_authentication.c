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

static char* sk_strdup(const char* s) {
    if (!s) return NULL;
    size_t len = strlen(s) + 1;
    char* dup = (char*)malloc(len);
    if (dup) memcpy(dup, s, len);
    return dup;
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
    size_t* out_body_len, uint32_t* out_eresult, int timeout_ms) {
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

    sk_steam_unified_messages_send_request(um, svc, method_name, request_body, request_body_len, 0,
        sk_auth_poll_session_status_cb, ctx);

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
    uint8_t* body = sk_unified_request_sync(client, "Authentication", "PollAuthSessionStatus",
        packed_buf, packed_size, &body_len, &eresult, 30000);
    free(packed_buf);
    sk_steam_unified_messages_remove_service(um, "Authentication");

    if (!body) {
        sk_debug_log_warn("Auth", "PollAuthSessionStatus timed out or failed (eresult=%u)", eresult);
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

static char* sk_fetch_password_rsa_key(sk_steam_client_t* client, const char* account_name, char** out_mod, char** out_exp) {
    if (!client || !account_name || !out_mod || !out_exp) return NULL;
    *out_mod = NULL;
    *out_exp = NULL;

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
        packed_buf, packed_size, &body_len, NULL, 30000);
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

    sk_steam_unified_messages_t* um = sk_steam_client_get_unified_messages(auth->client);
    if (!um) return NULL;

    sk_unified_service_t* svc = sk_steam_unified_messages_create_service(um, "Authentication");
    if (!svc) return NULL;

    CAuthenticationBeginAuthSessionViaQRRequest req = CAUTHENTICATION__BEGIN_AUTH_SESSION_VIA_QR__REQUEST__INIT;
    if (details->device_friendly_name) {
        req.device_friendly_name = details->device_friendly_name;
    }
    req.has_platform_type = true;
    req.platform_type = EAUTH_TOKEN_PLATFORM_TYPE__k_EAuthTokenPlatformType_SteamClient;
    req.website_id = "MobileClient";

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
    device_details->os_type = 1;
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
    uint8_t* body = sk_unified_request_sync(auth->client, "Authentication", "BeginAuthSessionViaQR",
        packed_buf, packed_size, &body_len, NULL, 30000);
    free(packed_buf);
    if (!body) {
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

    sk_steam_unified_messages_t* um = sk_steam_client_get_unified_messages(auth->client);
    if (!um) return NULL;

    sk_unified_service_t* svc = sk_steam_unified_messages_create_service(um, "Authentication");
    if (!svc) return NULL;

    CAuthenticationBeginAuthSessionViaCredentialsRequest req = CAUTHENTICATION__BEGIN_AUTH_SESSION_VIA_CREDENTIALS__REQUEST__INIT;
    req.account_name = details->username;
    if (details->password && details->password[0] && sk_crypto_is_available()) {
        char* mod = NULL;
        char* exp = NULL;
        char* pub_key = sk_fetch_password_rsa_key(auth->client, details->username, &mod, &exp);
        if (pub_key) {
            size_t enc_len = 0;
            uint8_t* enc = sk_crypto_rsa_encrypt((const uint8_t*)details->password, strlen(details->password),
                (const uint8_t*)pub_key, strlen(pub_key), &enc_len);
            if (enc) {
                req.encrypted_password = sk_crypto_base64_encode(enc, enc_len);
                req.has_encryption_timestamp = true;
                req.encryption_timestamp = (uint64_t)time(NULL);
                free(enc);
            }
            free(pub_key);
        }
        free(mod);
        free(exp);
    }
    if (details->device_friendly_name) {
        req.device_friendly_name = details->device_friendly_name;
    }
    req.has_remember_login = true;
    req.remember_login = details->is_remember_password;
    req.has_platform_type = true;
    req.platform_type = EAUTH_TOKEN_PLATFORM_TYPE__k_EAuthTokenPlatformType_SteamClient;
    req.has_persistence = true;
    req.persistence = details->is_persistent_session ?
        ESESSION_PERSISTENCE__k_ESessionPersistence_Persistent :
        ESESSION_PERSISTENCE__k_ESessionPersistence_Ephemeral;
    req.website_id = "MobileClient";

    size_t packed_size = cauthentication__begin_auth_session_via_credentials__request__get_packed_size(&req);
    uint8_t* packed_buf = (uint8_t*)malloc(packed_size);
    if (!packed_buf) {
        sk_steam_unified_messages_remove_service(um, "Authentication");
        return NULL;
    }
    cauthentication__begin_auth_session_via_credentials__request__pack(&req, packed_buf);

    size_t body_len = 0;
    uint8_t* body = sk_unified_request_sync(auth->client, "Authentication", "BeginAuthSessionViaCredentials",
        packed_buf, packed_size, &body_len, NULL, 30000);
    free(packed_buf);
    if (!body) {
        sk_steam_unified_messages_remove_service(um, "Authentication");
        return NULL;
    }

    CAuthenticationBeginAuthSessionViaCredentialsResponse* response =
        cauthentication__begin_auth_session_via_credentials__response__unpack(NULL, body_len, body);
    free(body);
    if (!response || !response->has_client_id || !response->has_request_id) {
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

sk_auth_poll_result_t* sk_credentials_auth_session_poll_wait_for_result(sk_credentials_auth_session_t* session) {
    if (!session) return NULL;
    sk_debug_log_info("Auth", "Polling Credentials Session (client_id=%llu)", (unsigned long long)session->client_id);

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

void sk_credentials_auth_session_destroy(sk_credentials_auth_session_t* session) {
    if (!session) return;
    sk_auth_session_details_destroy(session->details);
    free(session);
}
