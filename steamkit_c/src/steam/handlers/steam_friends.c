#include "steamkit/steam/handlers/steam_friends.h"
#include "steamkit/steam/handlers/client_msg_handler.h"
#include "steamkit/steam/steam_client.h"
#include "steamkit/steam/callbacks.h"
#include "steamkit/utils/debug_log.h"
#include "steamkit/utils/msg_util.h"
#include "steamkit/base/generated/steam_msg_friends.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

static void sk_steam_friends_handle_msg(struct sk_client_msg_handler* handler, const sk_packet_msg_t* packet_msg) {
    if (!handler || !packet_msg) return;

    uint32_t msg_type = sk_packet_msg_msg_type(packet_msg);
    switch (msg_type) {
        case SK_EMSG_CLIENT_FRIENDS_LIST: {
            size_t data_len = 0;
            const uint8_t* data = sk_packet_msg_data(packet_msg, &data_len);
            if (data && data_len > 20) {
                uint8_t wt;
                size_t offset;
                uint64_t value;
                if (sk_msg_find_field(data + 20, data_len - 20, 1, &wt, &offset, &value)) {
                    sk_debug_log_info("SteamFriends", "Friends list incremented: %llu", (unsigned long long)value);
                }
            }
            sk_debug_log_info("SteamFriends", "Received friends list");
            break;
        }
        case SK_EMSG_CLIENT_PERSONA_STATE: {
            size_t data_len = 0;
            const uint8_t* data = sk_packet_msg_data(packet_msg, &data_len);
            if (data && data_len > 20) {
                sk_debug_log_info("SteamFriends", "Received persona state update, %zu bytes", data_len - 20);
            }
            break;
        }
        case SK_EMSG_CLIENT_FRIENDS_GROUPS_LIST: {
            sk_debug_log_info("SteamFriends", "Received friends groups list");
            break;
        }
        case SK_EMSG_CLIENT_CHAT_INVITE: {
            sk_debug_log_info("SteamFriends", "Received chat invite");
            break;
        }
        case SK_EMSG_CLIENT_CHAT_JOIN: {
            sk_debug_log_info("SteamFriends", "Received chat join");
            break;
        }
        case SK_EMSG_CLIENT_CHAT_LEAVE: {
            sk_debug_log_info("SteamFriends", "Received chat leave");
            break;
        }
        default:
            break;
    }
}

typedef struct sk_steam_friends {
    struct sk_client_msg_handler base;
} sk_steam_friends_t;

sk_steam_friends_t* sk_steam_friends_create(void) {
    sk_steam_friends_t* friends = (sk_steam_friends_t*)calloc(1, sizeof(sk_steam_friends_t));
    if (friends) {
        friends->base.handle_msg = sk_steam_friends_handle_msg;
    }
    return friends;
}

void sk_steam_friends_destroy(sk_steam_friends_t* friends) {
    free(friends);
}

void sk_steam_friends_request_user_info(sk_steam_friends_t* friends, const sk_steam_id_t* steam_id) {
    (void)friends;
    (void)steam_id;
    sk_debug_log_info("SteamFriends", "Requesting user info");
}

void sk_steam_friends_set_persona_state(sk_steam_friends_t* friends, uint32_t persona_state) {
    (void)friends;
    (void)persona_state;
    sk_debug_log_info("SteamFriends", "Setting persona state: %u", persona_state);
}
