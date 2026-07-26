#include "steamkit/steam/authentication/steam_authentication.h"
#include "steamkit/steam/handlers/steam_unified_messages.h"
#include "steamkit/steam/steam_client.h"
#include "steamkit/utils/debug_log.h"
#include "steammessages_auth.steamclient.pb-c.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

static char* sk_strdup(const char* s) {
    if (!s) return NULL;
    size_t len = strlen(s) + 1;
    char* dup = (char*)malloc(len);
    if (dup) memcpy(dup, s, len);
    return dup;
}

typedef struct sk_auth_poll_req_ctx {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    bool done;
    uint8_t* body;
    size_t body_len;
} sk_auth_poll_req_ctx_t;

struct sk_qr_auth_session {
    sk_steam_client_t* client;
    uint64_t client_id;
    uint8_t request_id[20];
    int polling_interval_ms;
    char* challenge_url;
    sk_auth_session_details_t* details;
};

struct sk_credentials_auth_session {
    sk_steam_client_t* client;
    uint64_t client_id;
    uint8_t request_id[20];
    int polling_interval_ms;
    uint64_t steam_id;
    sk_auth_session_details_t* details;
};

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

static void sk_auth_poll_session_status_cb(void* user_data, const uint8_t* body, size_t body_len, uint32_t eresult) {
    sk_auth_poll_req_ctx_t* ctx = (sk_auth_poll_req_ctx_t*)user_data;
    (void)eresult;
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
    if (!packed_buf) return NULL;
    cauthentication__poll_auth_session_status__request__pack(&req, packed_buf);

    sk_auth_poll_req_ctx_t* ctx = sk_auth_poll_req_ctx_create();
    if (!ctx) {
        free(packed_buf);
        return NULL;
    }

    sk_steam_unified_messages_send_request(um, svc, "PollAuthSessionStatus", packed_buf, packed_size, 0,
        sk_auth_poll_session_status_cb, ctx);

    free(packed_buf);

    pthread_mutex_lock(&ctx->mutex);
    while (!ctx->done) {
        pthread_cond_wait(&ctx->cond, &ctx->mutex);
    }
    pthread_mutex_unlock(&ctx->mutex);

    uint8_t* result = ctx->body;
    if (out_body_len) *out_body_len = ctx->body_len;
    ctx->body = NULL;
    ctx->body_len = 0;
    sk_auth_poll_req_ctx_destroy(ctx);

    return result;
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

sk_qr_auth_session_t* sk_auth_begin_session_via_qr(sk_steam_authentication_t* auth, const sk_auth_session_details_t* details) {
    if (!auth || !details) return NULL;
    sk_qr_auth_session_t* session = (sk_qr_auth_session_t*)calloc(1, sizeof(sk_qr_auth_session_t));
    if (session) {
        session->client = auth->client;
        session->details = sk_auth_session_details_create(details->username, details->password);
        if (session->details) {
            session->details->device_friendly_name = sk_strdup(details->device_friendly_name);
            session->details->is_persistent_session = details->is_persistent_session;
        }
        sk_debug_log_info("Auth", "QR Session started");
    }
    return session;
}

sk_auth_poll_result_t* sk_qr_auth_session_poll_wait_for_result(sk_qr_auth_session_t* session) {
    if (!session) return NULL;
    sk_debug_log_info("Auth", "Polling QR Session");

    size_t body_len = 0;
    uint8_t* body = sk_auth_poll_auth_session_status(session->client, session->client_id, session->request_id, 20, &body_len);
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

    if (response->new_challenge_url && strlen(response->new_challenge_url) > 0) {
        free(session->challenge_url);
        session->challenge_url = sk_strdup(response->new_challenge_url);
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
    return NULL;
}

void sk_qr_auth_session_destroy(sk_qr_auth_session_t* session) {
    if (!session) return;
    free(session->challenge_url);
    sk_auth_session_details_destroy(session->details);
    free(session);
}

sk_credentials_auth_session_t* sk_auth_begin_session_via_credentials(sk_steam_authentication_t* auth, const sk_auth_session_details_t* details) {
    if (!auth || !details) return NULL;
    sk_credentials_auth_session_t* session = (sk_credentials_auth_session_t*)calloc(1, sizeof(sk_credentials_auth_session_t));
    if (session) {
        session->client = auth->client;
        session->details = sk_auth_session_details_create(details->username, details->password);
        if (session->details) {
            session->details->device_friendly_name = sk_strdup(details->device_friendly_name);
            session->details->is_persistent_session = details->is_persistent_session;
            session->details->authenticator = details->authenticator;
        }
        sk_debug_log_info("Auth", "Credentials Session started for user: %s", details->username ? details->username : "(null)");

        if (details->authenticator.get_code) {
            const char* code = details->authenticator.get_code(details->authenticator.user_data);
            sk_debug_log_info("Auth", "Received authenticator code: %s", code ? code : "(null)");
        }
    }
    return session;
}

sk_auth_poll_result_t* sk_credentials_auth_session_poll_wait_for_result(sk_credentials_auth_session_t* session) {
    if (!session) return NULL;
    sk_debug_log_info("Auth", "Polling Credentials Session");

    size_t body_len = 0;
    uint8_t* body = sk_auth_poll_auth_session_status(session->client, session->client_id, session->request_id, 20, &body_len);
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
    return NULL;
}

void sk_credentials_auth_session_destroy(sk_credentials_auth_session_t* session) {
    if (!session) return;
    sk_auth_session_details_destroy(session->details);
    free(session);
}
