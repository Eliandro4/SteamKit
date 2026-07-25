#include "steamkit/steam/handlers/steam_user.h"
#include "steamkit/steam/handlers/client_msg_handler.h"
#include "steamkit/steam/steam_client.h"
#include "steamkit/steam/callbacks.h"
#include "steamkit/utils/debug_log.h"
#include "steamkit/utils/msg_util.h"
#include "steamkit/base/generated/steam_msg_user.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

static char* sk_strdup(const char* s) {
    if (!s) return NULL;
    size_t len = strlen(s) + 1;
    char* dup = (char*)malloc(len);
    if (dup) memcpy(dup, s, len);
    return dup;
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

static void sk_steam_user_handle_msg(struct sk_client_msg_handler* handler, const sk_packet_msg_t* packet_msg) {
    if (!handler || !packet_msg) return;

    typedef struct sk_steam_user sk_steam_user_t;
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
        case SK_EMSG_CLIENT_PERSONAS: {
            sk_debug_log_info("SteamUser", "Received personas list");
            break;
        }
        default:
            break;
    }
}

typedef struct sk_steam_user {
    struct sk_client_msg_handler base;
    sk_log_on_details_t* logon_details;
} sk_steam_user_t;

sk_steam_user_t* sk_steam_user_create(void) {
    sk_steam_user_t* user = (sk_steam_user_t*)calloc(1, sizeof(sk_steam_user_t));
    if (user) {
        user->base.handle_msg = sk_steam_user_handle_msg;
    }
    return user;
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
    
    sk_debug_log_info("SteamUser", "Logon requested for user: %s", details->username ? details->username : "(null)");
}

void sk_steam_user_log_off(sk_steam_user_t* user) {
    if (!user) return;
    sk_debug_log_info("SteamUser", "Logoff requested");
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
