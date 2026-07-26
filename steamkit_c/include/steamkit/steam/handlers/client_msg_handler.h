#ifndef STEAMKIT_STEAM_HANDLERS_CLIENT_MSG_HANDLER_H
#define STEAMKIT_STEAM_HANDLERS_CLIENT_MSG_HANDLER_H

#include <stdint.h>
#include <stdbool.h>
#include "steamkit/base/packet_base.h"
#include "steamkit/steam/steam_client.h"

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
typedef struct sk_client_msg_handler sk_client_msg_handler_t;
typedef struct sk_steam_client sk_steam_client_t;

// Client message handler interface - mirrors C# ClientMsgHandler
typedef enum sk_handler_type {
    SK_HANDLER_UNKNOWN = 0,
    SK_HANDLER_STEAM_APPS,
    SK_HANDLER_STEAM_CONTENT,
    SK_HANDLER_STEAM_CLOUD,
    SK_HANDLER_STEAM_FRIENDS,
    SK_HANDLER_STEAM_USER,
    SK_HANDLER_STEAM_GC,
    SK_HANDLER_STEAM_GAME_SERVER,
    SK_HANDLER_STEAM_USER_STATS,
    SK_HANDLER_STEAM_MASTER_SERVER,
    SK_HANDLER_STEAM_WORKSHOP,
    SK_HANDLER_STEAM_UNIFIED_MESSAGES,
    SK_HANDLER_STEAM_SCREENSHOTS,
    SK_HANDLER_STEAM_MATCHMAKING,
    SK_HANDLER_STEAM_NETWORKING,
    SK_HANDLER_STEAM_AUTH_TICKET,
    SK_HANDLER_CM_CLIENT,
    SK_HANDLER_STEAM_PUBLISHED_FILE,
} sk_handler_type_t;

struct sk_client_msg_handler {
    void (*handle_msg)(struct sk_client_msg_handler* handler, const sk_packet_msg_t* packet_msg);
    sk_steam_client_t* client;
    bool expect_disconnection;
    sk_handler_type_t handler_type;
};

// Handler functions
void sk_client_msg_handler_handle_msg(sk_client_msg_handler_t* handler, const sk_packet_msg_t* packet_msg);
void sk_client_msg_handler_setup(sk_client_msg_handler_t* handler, sk_steam_client_t* client);
bool sk_client_msg_handler_get_expect_disconnection(const sk_client_msg_handler_t* handler);
void sk_client_msg_handler_set_expect_disconnection(sk_client_msg_handler_t* handler, bool expect);
void sk_client_msg_handler_destroy(sk_client_msg_handler_t* handler);

#ifdef __cplusplus
}
#endif

#endif // STEAMKIT_STEAM_HANDLERS_CLIENT_MSG_HANDLER_H
