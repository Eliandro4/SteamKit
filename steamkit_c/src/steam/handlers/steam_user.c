#define _GNU_SOURCE
#include "steamkit/steam/handlers/steam_user.h"
#include "steamkit/steam/handlers/client_msg_handler.h"
#include "steamkit/steam/handlers/steam_unified_messages.h"
#include "steamkit/steam/steam_client.h"
#include "steamkit/steam/callbacks.h"
#include "steamkit/utils/debug_log.h"
#include "steamkit/utils/msg_util.h"
#include "steamkit/utils/crypto_helper.h"
#include "steamkit/base/generated/steam_msg_user.h"
#include "steamkit/steam/handlers/client_msg_protobuf.h"
#include "steamkit/steam/authentication/steam_authentication.h"
#include "steammessages_clientserver_login.pb-c.h"
#include "steammessages_clientserver.pb-c.h"
#include "steammessages_auth.steamclient.pb-c.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <time.h>

static char* sk_strdup(const char* s) {
    if (!s) return NULL;
    size_t len = strlen(s) + 1;
    char* dup = (char*)malloc(len);
    if (dup) memcpy(dup, s, len);
    return dup;
}

static bool sk_logon_use_rsa_encryption(const sk_log_on_details_t* details) {
    return details && details->password && details->password[0] && sk_crypto_is_available();
}

typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    bool done;
    char* public_key;
} sk_rsa_req_ctx_t;

static void sk_rsa_response_cb(void* user_data, const uint8_t* body, size_t body_len, uint32_t eresult) {
    (void)eresult;
    sk_rsa_req_ctx_t* ctx = (sk_rsa_req_ctx_t*)user_data;
    if (body && body_len > 0) {
        CAuthenticationGetPasswordRSAPublicKeyResponse* resp =
            cauthentication__get_password_rsapublic_key__response__unpack(NULL, body_len, body);
        if (resp) {
            if (resp->publickey_mod && resp->publickey_exp) {
                size_t len = strlen(resp->publickey_mod) + 1 + strlen(resp->publickey_exp) + 1;
                ctx->public_key = (char*)malloc(len);
                if (ctx->public_key) {
                    snprintf(ctx->public_key, len, "%s|%s", resp->publickey_mod, resp->publickey_exp);
                }
            }
            cauthentication__get_password_rsapublic_key__response__free_unpacked(resp, NULL);
        }
    }
    pthread_mutex_lock(&ctx->mutex);
    ctx->done = true;
    pthread_cond_signal(&ctx->cond);
    pthread_mutex_unlock(&ctx->mutex);
}

static char* sk_steam_user_fetch_rsa_key(sk_steam_client_t* client, const char* account_name) {
    if (!client || !account_name) return NULL;

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

    sk_rsa_req_ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    pthread_mutex_init(&ctx.mutex, NULL);
    pthread_cond_init(&ctx.cond, NULL);

    sk_steam_unified_messages_send_request(um, svc, "GetPasswordRSAPublicKey", packed_buf, packed_size, 0,
        sk_rsa_response_cb, &ctx);

    free(packed_buf);

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += 10;

    pthread_mutex_lock(&ctx.mutex);
    while (!ctx.done) {
        if (pthread_cond_timedwait(&ctx.cond, &ctx.mutex, &ts) != 0) {
            break;
        }
    }
    pthread_mutex_unlock(&ctx.mutex);

    pthread_mutex_destroy(&ctx.mutex);
    pthread_cond_destroy(&ctx.cond);

    sk_steam_unified_messages_remove_service(um, "Authentication");
    return ctx.public_key;
}

typedef struct sk_steam_user {
    struct sk_client_msg_handler base;
    sk_log_on_details_t* logon_details;
} sk_steam_user_t;

static void sk_steam_user_handle_msg(struct sk_client_msg_handler* handler, const sk_packet_msg_t* packet_msg) {
    if (!handler || !packet_msg) return;

    sk_steam_user_t* user = (sk_steam_user_t*)handler;

    uint32_t msg_type = sk_packet_msg_msg_type(packet_msg);
    switch (msg_type) {
        case SK_EMSG_CLIENT_PERSONA_STATE: {
            sk_debug_log_info("SteamUser", "Received persona state update");
            size_t data_len = 0;
            const uint8_t* data = sk_packet_msg_data(packet_msg, &data_len);
            if (data && data_len > 20) {
                uint8_t wt;
                size_t offset;
                uint64_t value;
                if (sk_msg_find_field(data + 20, data_len - 20, 2, &wt, &offset, &value)) {
                    sk_debug_log_info("SteamUser", "Persona state: %llu", (unsigned long long)value);
                }
            }
            break;
        }
        case SK_EMSG_CLIENT_LOGON_RESPONSE: {
            sk_debug_log_info("SteamUser", "Received logon response");
            size_t data_len = 0;
            const uint8_t* data = sk_packet_msg_data(packet_msg, &data_len);
            CMsgClientLogonResponse* resp = cmsg_client_logon_response__unpack(NULL, data_len, data);
            if (resp) {
                sk_logged_on_callback_t* cb = sk_logged_on_callback_create(resp->eresult);
                if (cb && user->base.client) {
                    sk_steam_client_post_callback(user->base.client, SK_CLIENT_CALLBACK_LOGGED_ON, 0, cb);
                }
                cmsg_client_logon_response__free_unpacked(resp, NULL);
            }
            break;
        }
        case SK_EMSG_CLIENT_LOGGED_OFF: {
            sk_debug_log_info("SteamUser", "Received logged off");
            size_t data_len = 0;
            const uint8_t* data = sk_packet_msg_data(packet_msg, &data_len);
            CMsgClientLoggedOff* logged_off = cmsg_client_logged_off__unpack(NULL, data_len, data);
            if (logged_off) {
                sk_logged_off_callback_t* cb = sk_logged_off_callback_create(logged_off->eresult);
                if (cb && user->base.client) {
                    sk_steam_client_post_callback(user->base.client, SK_CLIENT_CALLBACK_LOGGED_OFF, 0, cb);
                }
                cmsg_client_logged_off__free_unpacked(logged_off, NULL);
            }
            break;
        }
        case SK_EMSG_CLIENT_SESSION_TOKEN: {
            sk_debug_log_info("SteamUser", "Received session token");
            size_t data_len = 0;
            const uint8_t* data = sk_packet_msg_data(packet_msg, &data_len);
            CMsgClientSessionToken* sess_token = cmsg_client_session_token__unpack(NULL, data_len, data);
            if (sess_token) {
                sk_session_token_callback_t* cb = sk_session_token_callback_create(sess_token->token);
                if (cb && user->base.client) {
                    sk_steam_client_post_callback(user->base.client, SK_CLIENT_CALLBACK_SESSION_TOKEN, 0, cb);
                }
                cmsg_client_session_token__free_unpacked(sess_token, NULL);
            }
            break;
        }
        case SK_EMSG_CLIENT_ACCOUNT_INFO: {
            sk_debug_log_info("SteamUser", "Received account info");
            size_t data_len = 0;
            const uint8_t* data = sk_packet_msg_data(packet_msg, &data_len);
            CMsgClientAccountInfo* acc_info = cmsg_client_account_info__unpack(NULL, data_len, data);
            if (acc_info) {
                sk_account_info_callback_t* cb = sk_account_info_callback_create(
                    acc_info->persona_name,
                    acc_info->ip_country,
                    acc_info->count_authed_computers,
                    acc_info->account_flags
                );
                if (cb && user->base.client) {
                    sk_steam_client_post_callback(user->base.client, SK_CLIENT_CALLBACK_ACCOUNT_INFO, 0, cb);
                }
                cmsg_client_account_info__free_unpacked(acc_info, NULL);
            }
            break;
        }
        default:
            break;
    }
}

sk_steam_user_t* sk_steam_user_create(void) {
    sk_steam_user_t* user = (sk_steam_user_t*)calloc(1, sizeof(sk_steam_user_t));
    if (user) {
        user->base.handle_msg = sk_steam_user_handle_msg;
        user->base.handler_type = SK_HANDLER_STEAM_USER;
    }
    return user;
}

sk_log_on_details_t* sk_log_on_details_create(void) {
    return (sk_log_on_details_t*)calloc(1, sizeof(sk_log_on_details_t));
}

void sk_log_on_details_destroy(sk_log_on_details_t* details) {
    if (!details) return;
    free(details->username);
    free(details->password);
    free(details->auth_code);
    free(details->two_factor_code);
    free(details->access_token);
    free(details->machine_name);
    free(details);
}

void sk_steam_user_log_on(sk_steam_user_t* user, const sk_log_on_details_t* details) {
    if (!user || !details) return;
    if (user->logon_details) {
        sk_log_on_details_destroy(user->logon_details);
    }
    user->logon_details = sk_log_on_details_create();
    if (!user->logon_details) return;
    
    user->logon_details->username = details->username ? sk_strdup(details->username) : NULL;
    user->logon_details->password = details->password ? sk_strdup(details->password) : NULL;
    user->logon_details->cell_id = details->cell_id;
    user->logon_details->login_id = details->login_id;
    user->logon_details->auth_code = details->auth_code ? sk_strdup(details->auth_code) : NULL;
    user->logon_details->two_factor_code = details->two_factor_code ? sk_strdup(details->two_factor_code) : NULL;
    user->logon_details->should_remember_password = details->should_remember_password;
    user->logon_details->access_token = details->access_token ? sk_strdup(details->access_token) : NULL;
    user->logon_details->account_instance = details->account_instance;
    user->logon_details->machine_name = details->machine_name ? sk_strdup(details->machine_name) : NULL;

    if (!user->base.client) {
        sk_debug_log_warn("SteamUser", "No SteamClient attached, cannot send logon message.");
        return;
    }

    char* encrypted_password = NULL;
    if (sk_logon_use_rsa_encryption(user->logon_details)) {
        char* pub_key = sk_steam_user_fetch_rsa_key(user->base.client, user->logon_details->username);
        if (pub_key) {
            size_t enc_len = 0;
            uint8_t* enc = sk_crypto_rsa_encrypt((const uint8_t*)user->logon_details->password,
                strlen(user->logon_details->password), (const uint8_t*)pub_key, strlen(pub_key), &enc_len);
            if (enc) {
                encrypted_password = sk_crypto_base64_encode(enc, enc_len);
                free(enc);
                sk_debug_log_info("SteamUser", "Password encrypted with RSA public key");
            }
            free(pub_key);
        }
    }

    sk_client_msg_protobuf_t* msg = sk_client_msg_protobuf_create(SK_EMSG_CLIENT_LOG_ON);
    if (!msg) {
        free(encrypted_password);
        return;
    }

    CMsgClientLogon logon_msg = CMSG_CLIENT_LOGON__INIT;
    logon_msg.account_name = user->logon_details->username;
    logon_msg.password = encrypted_password ? encrypted_password : user->logon_details->password;
    
    if (user->logon_details->machine_name) {
        logon_msg.machine_name = user->logon_details->machine_name;
    }

    if (user->logon_details->cell_id != 0) {
        logon_msg.has_cell_id = true;
        logon_msg.cell_id = user->logon_details->cell_id;
    }
    
    if (user->logon_details->access_token) {
        logon_msg.web_logon_nonce = user->logon_details->access_token;
    }
    
    if (user->logon_details->two_factor_code) {
        logon_msg.two_factor_code = user->logon_details->two_factor_code;
    }
    
    if (user->logon_details->auth_code) {
        logon_msg.auth_code = user->logon_details->auth_code;
    }

    logon_msg.protocol_version = 65580;
    logon_msg.has_protocol_version = true;
    logon_msg.client_os_type = 10;
    logon_msg.has_client_os_type = true;
    
    size_t packed_size = cmsg_client_logon__get_packed_size(&logon_msg);
    uint8_t* packed_buf = (uint8_t*)malloc(packed_size);
    if (packed_buf) {
        cmsg_client_logon__pack(&logon_msg, packed_buf);
        sk_client_msg_protobuf_set_body(msg, packed_buf, packed_size);
        free(packed_buf);
    }
    
    sk_packet_msg_t* pkt = sk_packet_msg_create_from_client_msg_protobuf(msg);
    if (pkt) {
        sk_steam_client_send(user->base.client, pkt);
        sk_packet_msg_destroy(pkt);
    }
    
    sk_client_msg_protobuf_destroy(msg);
    free(encrypted_password);
    sk_debug_log_info("SteamUser", "Logon message sent for user: %s", details->username ? details->username : "(null)");
}

void sk_steam_user_log_off(sk_steam_user_t* user) {
    if (!user) return;
    sk_debug_log_info("SteamUser", "Logoff requested");
    
    if (user->base.client) {
        sk_client_msg_protobuf_t* msg = sk_client_msg_protobuf_create(SK_EMSG_CLIENT_LOG_OFF);
        if (msg) {
            sk_packet_msg_t* pkt = sk_packet_msg_create_from_client_msg_protobuf(msg);
            if (pkt) {
                sk_steam_client_send(user->base.client, pkt);
                sk_packet_msg_destroy(pkt);
            }
            sk_client_msg_protobuf_destroy(msg);
        }
    }
    
    if (user->logon_details) {
        sk_log_on_details_destroy(user->logon_details);
        user->logon_details = NULL;
    }
}

void sk_steam_user_destroy(sk_steam_user_t* user) {
    if (!user) return;
    if (user->logon_details) {
        sk_log_on_details_destroy(user->logon_details);
    }
    free(user);
}
