#ifndef STEAMKIT_STEAM_CM_CLIENT_H
#define STEAMKIT_STEAM_CM_CLIENT_H

#include <stdint.h>
#include <stdbool.h>
#include "steamkit/networking/connection.h"
#include "steamkit/types/steam_id.h"
#include "steamkit/types/game_id.h"
#include "steamkit/base/emsg.h"

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
typedef struct sk_steam_client sk_steam_client_t;
typedef struct sk_steam_configuration sk_steam_configuration_t;

typedef enum sk_cm_server_type {
    SK_CM_SERVER_TYPE_WEBSOCKET,
    SK_CM_SERVER_TYPE_TCP,
} sk_cm_server_type_t;

typedef struct sk_cm_server {
    const char* host;
    uint16_t port;
    sk_cm_server_type_t type;
} sk_cm_server_t;

// Callback function types
typedef void (*sk_cm_client_callback_fn)(void* user_data);

// CM Client - mirrors C# CMClient (base class for SteamClient)
typedef struct sk_cm_client sk_cm_client_t;

sk_cm_client_t* sk_cm_client_create(sk_steam_configuration_t* config, const char* identifier);
void sk_cm_client_destroy(sk_cm_client_t* client);

bool sk_cm_client_is_connected(const sk_cm_client_t* client);
void sk_cm_client_connect(sk_cm_client_t* client);
void sk_cm_client_disconnect(sk_cm_client_t* client, bool user_initiated);

const sk_steam_id_t* sk_cm_client_steam_id(const sk_cm_client_t* client);
uint64_t sk_cm_client_session_token(const sk_cm_client_t* client);
const char* sk_cm_client_local_ip(const sk_cm_client_t* client);
sk_connection_t* sk_cm_client_connection(const sk_cm_client_t* client);

void sk_cm_client_set_callback(sk_cm_client_t* client, sk_cm_client_callback_fn fn, void* user_data);
void sk_cm_client_post_callback(sk_cm_client_t* client, void* callback_msg);

void sk_cm_client_set_steam_client(sk_cm_client_t* client, sk_steam_client_t* steam_client);
sk_steam_client_t* sk_cm_client_get_steam_client(const sk_cm_client_t* client);

void sk_cm_client_on_connected(sk_cm_client_t* client);
void sk_cm_client_on_disconnected(sk_cm_client_t* client, bool user_initiated);
void sk_cm_client_on_packet_received(sk_cm_client_t* client, const sk_packet_msg_t* packet_msg);

void sk_cm_client_set_server_list(sk_cm_client_t* client, const sk_cm_server_t* servers, size_t count);
const sk_cm_server_t* sk_cm_client_get_server_list(const sk_cm_client_t* client, size_t* out_count);

#ifdef __cplusplus
}
#endif

#endif // STEAMKIT_STEAM_CM_CLIENT_H
