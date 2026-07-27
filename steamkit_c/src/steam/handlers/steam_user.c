#define _GNU_SOURCE
#include "steamkit/steam/handlers/steam_user.h"
#include "steamkit/steam/handlers/client_msg_handler.h"
#include "steamkit/steam/handlers/steam_unified_messages.h"
#include "steamkit/steam/steam_client.h"
#include "steamkit/steam/callbacks.h"
#include "steamkit/types/steam_id.h"
#include "steamkit/utils/debug_log.h"
#include "steamkit/utils/msg_util.h"
#include "steamkit/utils/net_helpers.h"
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
#include <unistd.h>

static char* sk_strdup(const char* s) {
    if (!s) return NULL;
    size_t len = strlen(s) + 1;
    char* dup = (char*)malloc(len);
    if (dup) memcpy(dup, s, len);
    return dup;
}

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

static uint8_t* sk_generate_machine_id(size_t* out_len) {
    if (!out_len) return NULL;
    uint8_t* blob = (uint8_t*)malloc(8);
    if (!blob) { *out_len = 0; return NULL; }
    uint64_t rnd = sk_net_random_uint64();
    for (int i = 0; i < 8; ++i) {
        blob[i] = (uint8_t)((rnd >> (i * 8)) & 0xFF);
    }
    *out_len = 8;
    return blob;
}

static CMsgIPAddress* sk_create_obfuscated_private_ip(uint32_t login_id) {
    CMsgIPAddress* ip = (CMsgIPAddress*)calloc(1, sizeof(CMsgIPAddress));
    if (!ip) return NULL;
    cmsg_ipaddress__init(ip);
    if (login_id != 0) {
        ip->ip_case = CMSG_IPADDRESS__IP_V4;
        ip->v4 = login_id ^ 0xBAADF00D;
    } else {
        uint32_t local_ip = sk_net_get_local_ip();
        ip->ip_case = CMSG_IPADDRESS__IP_V4;
        ip->v4 = local_ip ^ 0xBAADF00D;
    }
    return ip;
}

typedef struct sk_steam_user {
    struct sk_client_msg_handler base;
    sk_log_on_details_t* logon_details;
} sk_steam_user_t;

static const uint8_t* sk_steam_user_get_message_body(const sk_packet_msg_t* packet_msg, size_t* out_len) {
    if (!packet_msg || !out_len) return NULL;
    *out_len = 0;

    if (sk_packet_msg_is_proto(packet_msg)) {
        sk_client_msg_protobuf_t* proto = sk_client_msg_protobuf_create_from_packet(packet_msg);
        if (!proto) return NULL;
        const uint8_t* body = sk_client_msg_protobuf_get_body(proto, out_len);
        if (!body || *out_len == 0) {
            sk_client_msg_protobuf_destroy(proto);
            return NULL;
        }
        uint8_t* copy = (uint8_t*)malloc(*out_len);
        if (!copy) {
            sk_client_msg_protobuf_destroy(proto);
            *out_len = 0;
            return NULL;
        }
        memcpy(copy, body, *out_len);
        sk_client_msg_protobuf_destroy(proto);
        return copy;
    }

    return sk_packet_msg_data(packet_msg, out_len);
}

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
            const uint8_t* data = sk_steam_user_get_message_body(packet_msg, &data_len);
            CMsgClientLogonResponse* resp = data ? cmsg_client_logon_response__unpack(NULL, data_len, data) : NULL;
            if (sk_packet_msg_is_proto(packet_msg)) {
                free((void*)data);
            }
            if (resp) {
                sk_logged_on_callback_t* cb = sk_logged_on_callback_create(resp->eresult);
                if (cb) {
                    cb->extended_result = resp->has_eresult_extended ? resp->eresult_extended : 0;
                    if (resp->has_client_supplied_steamid && user->base.client) {
                        sk_steam_id_t* sid = sk_steam_id_create(resp->client_supplied_steamid);
                        sk_cm_client_set_steam_id(sk_steam_client_get_cm_client(user->base.client), sid);
                        cb->client_steam_id = sid;
                    }
                    if (user->base.client) {
                        sk_steam_client_post_callback(user->base.client, SK_CLIENT_CALLBACK_LOGGED_ON, 0, cb);
                    }
                }
                cmsg_client_logon_response__free_unpacked(resp, NULL);
            }
            break;
        }
        case SK_EMSG_CLIENT_LOGGED_OFF: {
            sk_debug_log_info("SteamUser", "Received logged off");
            size_t data_len = 0;
            const uint8_t* data = sk_steam_user_get_message_body(packet_msg, &data_len);
            CMsgClientLoggedOff* logged_off = data ? cmsg_client_logged_off__unpack(NULL, data_len, data) : NULL;
            if (sk_packet_msg_is_proto(packet_msg)) {
                free((void*)data);
            }
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
            const uint8_t* data = sk_steam_user_get_message_body(packet_msg, &data_len);
            CMsgClientSessionToken* sess_token = data ? cmsg_client_session_token__unpack(NULL, data_len, data) : NULL;
            if (sk_packet_msg_is_proto(packet_msg)) {
                free((void*)data);
            }
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
            const uint8_t* data = sk_steam_user_get_message_body(packet_msg, &data_len);
            CMsgClientAccountInfo* acc_info = data ? cmsg_client_account_info__unpack(NULL, data_len, data) : NULL;
            if (sk_packet_msg_is_proto(packet_msg)) {
                free((void*)data);
            }
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
    free(details->guard_data);
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
    user->logon_details->account_instance = details->account_instance ? details->account_instance : 1;
    user->logon_details->machine_name = details->machine_name ? sk_strdup(details->machine_name) : NULL;
    user->logon_details->guard_data = details->guard_data ? sk_strdup(details->guard_data) : NULL;
    user->logon_details->authenticator = details->authenticator;

    if (!user->base.client) {
        sk_debug_log_warn("SteamUser", "No SteamClient attached, cannot send logon message.");
        return;
    }

    if (!sk_steam_client_is_connected(user->base.client)) {
        sk_debug_log_warn("SteamUser", "SteamClient is not connected, cannot send logon message.");
        return;
    }

    sk_client_msg_protobuf_t* msg = sk_client_msg_protobuf_create(SK_EMSG_CLIENT_LOG_ON);
    if (!msg) {
        return;
    }

    CMsgProtoBufHeader* hdr = sk_client_msg_protobuf_header(msg);
    if (hdr) {
        hdr->has_client_sessionid = true;
        hdr->client_sessionid = 0;
        if (user->logon_details->steam_id != 0) {
            hdr->has_steamid = true;
            hdr->steamid = user->logon_details->steam_id;
        }
    }

    CMsgClientLogon logon_msg = CMSG_CLIENT_LOGON__INIT;
    logon_msg.account_name = user->logon_details->username;
    logon_msg.password = user->logon_details->password;
    
    if (user->logon_details->machine_name) {
        logon_msg.machine_name = user->logon_details->machine_name;
    }

    if (user->logon_details->cell_id != 0) {
        logon_msg.has_cell_id = true;
        logon_msg.cell_id = user->logon_details->cell_id;
    }
    
    if (user->logon_details->access_token) {
        logon_msg.access_token = user->logon_details->access_token;
    }

    if (user->logon_details->two_factor_code) {
        logon_msg.two_factor_code = user->logon_details->two_factor_code;
    }

    if (user->logon_details->auth_code) {
        logon_msg.auth_code = user->logon_details->auth_code;
    }

    logon_msg.protocol_version = 65581;
    logon_msg.has_protocol_version = true;
    logon_msg.client_os_type = sk_get_os_type();
    logon_msg.has_client_os_type = true;
    logon_msg.client_package_version = 1771;
    logon_msg.has_client_package_version = true;
    logon_msg.supports_rate_limit_response = true;
    logon_msg.has_supports_rate_limit_response = true;
    logon_msg.client_language = "english";
    logon_msg.has_should_remember_password = true;
    logon_msg.should_remember_password = user->logon_details->should_remember_password;

    if (user->logon_details->login_id != 0) {
        CMsgIPAddress* obf_ip = sk_create_obfuscated_private_ip(user->logon_details->login_id);
        if (obf_ip) {
            logon_msg.obfuscated_private_ip = obf_ip;
            logon_msg.has_deprecated_obfustucated_private_ip = true;
            logon_msg.deprecated_obfustucated_private_ip = obf_ip->v4;
        }
    } else {
        CMsgIPAddress* obf_ip = sk_create_obfuscated_private_ip(0);
        if (obf_ip) {
            logon_msg.obfuscated_private_ip = obf_ip;
            logon_msg.has_deprecated_obfustucated_private_ip = true;
            logon_msg.deprecated_obfustucated_private_ip = obf_ip->v4;
        }
    }

    size_t machine_id_len = 0;
    uint8_t* machine_id = sk_generate_machine_id(&machine_id_len);
    CMsgIPAddress* obf_ip = logon_msg.obfuscated_private_ip;
    if (machine_id && machine_id_len > 0) {
        logon_msg.has_machine_id = true;
        logon_msg.machine_id.data = machine_id;
        logon_msg.machine_id.len = machine_id_len;
    }
    
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
    free(machine_id);
    free(obf_ip);
    sk_debug_log_info("SteamUser", "Logon message sent for user: %s", details->username ? details->username : "(null)");
}

void sk_steam_user_log_on_with_password(sk_steam_user_t* user, const char* username, const char* password, sk_authenticator_t authenticator) {
    if (!user || !username || !password) return;
    if (!user->base.client) {
        sk_debug_log_warn("SteamUser", "No SteamClient attached, cannot authenticate.");
        return;
    }
    if (!sk_steam_client_is_connected(user->base.client)) {
        sk_debug_log_warn("SteamUser", "SteamClient is not connected, cannot authenticate.");
        return;
    }

    sk_steam_authentication_t* auth = sk_steam_authentication_create(user->base.client);
    if (!auth) {
        sk_debug_log_warn("SteamUser", "Failed to create authentication handler.");
        return;
    }

    char* mod = NULL;
    char* exp = NULL;
    uint64_t rsa_timestamp = 0;
    char* pub_key = sk_fetch_password_rsa_key(user->base.client, username, &mod, &exp, &rsa_timestamp);
    if (!pub_key) {
        sk_debug_log_warn("SteamUser", "Failed to fetch RSA public key.");
        sk_steam_authentication_destroy(auth);
        return;
    }

    size_t enc_len = 0;
    uint8_t* enc = sk_crypto_rsa_encrypt((const uint8_t*)password, strlen(password), (const uint8_t*)pub_key, strlen(pub_key), &enc_len);
    if (!enc) {
        sk_debug_log_warn("SteamUser", "RSA encryption of password failed.");
        free(pub_key);
        free(mod);
        free(exp);
        sk_steam_authentication_destroy(auth);
        return;
    }
    char* encrypted_password = sk_crypto_base64_encode(enc, enc_len);
    free(enc);
    free(pub_key);
    free(mod);
    free(exp);
    if (!encrypted_password) {
        sk_debug_log_warn("SteamUser", "Base64 encoding of encrypted password failed.");
        sk_steam_authentication_destroy(auth);
        return;
    }

    sk_auth_session_details_t* details = sk_auth_session_details_create(username, password);
    if (!details) {
        sk_debug_log_warn("SteamUser", "Failed to create auth session details.");
        free(encrypted_password);
        sk_steam_authentication_destroy(auth);
        return;
    }
    details->authenticator = authenticator;
    if (rsa_timestamp != 0) {
        details->guard_data = (char*)malloc(32);
        if (details->guard_data) {
            snprintf(details->guard_data, 32, "%llu", (unsigned long long)rsa_timestamp);
        }
    }

    sk_credentials_auth_session_t* session = sk_auth_begin_session_via_credentials(auth, details);
    sk_auth_session_details_destroy(details);
    if (!session) {
        sk_debug_log_warn("SteamUser", "Failed to begin auth session via credentials.");
        free(encrypted_password);
        sk_steam_authentication_destroy(auth);
        return;
    }

    sk_auth_poll_result_t* result = sk_credentials_auth_session_poll_wait_for_result(session);
    sk_credentials_auth_session_destroy(session);
    free(encrypted_password);

    if (!result || !result->access_token) {
        sk_debug_log_warn("SteamUser", "Authentication failed or no access token received.");
        sk_auth_poll_result_destroy(result);
        sk_steam_authentication_destroy(auth);
        return;
    }

    sk_log_on_details_t* logon_details = sk_log_on_details_create();
    if (!logon_details) {
        sk_debug_log_warn("SteamUser", "Failed to create logon details.");
        sk_auth_poll_result_destroy(result);
        sk_steam_authentication_destroy(auth);
        return;
    }
    logon_details->username = sk_strdup(username);
    logon_details->access_token = sk_strdup(result->access_token);
    if (result->refresh_token) {
        logon_details->password = sk_strdup(result->refresh_token);
    }
    logon_details->should_remember_password = true;

    sk_steam_user_log_on(user, logon_details);
    sk_log_on_details_destroy(logon_details);
    sk_auth_poll_result_destroy(result);
    sk_steam_authentication_destroy(auth);
    sk_debug_log_info("SteamUser", "Authentication successful, logon sent for user: %s", username);
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
