#ifndef STEAMKIT_STEAM_STEAM_CLIENT_H
#define STEAMKIT_STEAM_STEAM_CLIENT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "steamkit/steam/cm_client.h"
#include "steamkit/base/client_msg.h"
#include "steamkit/types/job_id.h"

// Forward declarations needed for handler interface
struct sk_client_msg_handler;
struct sk_steam_client;

#ifdef __cplusplus
extern "C" {
#endif

// SteamClient callback types for result delivery
typedef enum sk_client_callback_type {
    SK_CLIENT_CALLBACK_CONNECTED = 1,
    SK_CLIENT_CALLBACK_DISCONNECTED,
    SK_CLIENT_CALLBACK_LOGGED_ON,
    SK_CLIENT_CALLBACK_LOGGED_OFF,
    SK_CLIENT_CALLBACK_SESSION_TOKEN,
    SK_CLIENT_CALLBACK_ACCOUNT_INFO,
    SK_CLIENT_CALLBACK_PERSONA_STATE,
    SK_CLIENT_CALLBACK_FRIEND_RELATIONSHIP,
    SK_CLIENT_CALLBACK_LICENSE_LIST,
    SK_CLIENT_CALLBACK_FREE_LICENSE,
    SK_CLIENT_CALLBACK_APP_OWNERSHIP_TICKET,
    SK_CLIENT_CALLBACK_DEPOT_KEY,
    SK_CLIENT_CALLBACK_PICS_ACCESS_TOKEN,
    SK_CLIENT_CALLBACK_PICS_CHANGES,
    SK_CLIENT_CALLBACK_PICS_PRODUCT_INFO,
    SK_CLIENT_CALLBACK_PRIVATE_BETA,
    SK_CLIENT_CALLBACK_CDN_SERVER_LIST,
    SK_CLIENT_CALLBACK_MANIFEST_REQUEST_CODE,
    SK_CLIENT_CALLBACK_CDN_AUTH_TOKEN,
    SK_CLIENT_CALLBACK_UGC_DETAILS,
    SK_CLIENT_CALLBACK_PUBLISHED_FILE_DETAILS,
    SK_CLIENT_CALLBACK_SERVICE_METHOD_RESPONSE,
    SK_CLIENT_CALLBACK_GC_MESSAGE,
    SK_CLIENT_CALLBACK_LOBBY_MATCHMAKING,
} sk_client_callback_type_t;

// SteamClient - mirrors C# SteamClient (the main entry point)
// Inherits from CMClient via composition
typedef struct sk_steam_client sk_steam_client_t;

// Creates a new SteamClient with default configuration
sk_steam_client_t* sk_steam_client_create(void);

// Creates a new SteamClient with a specific configuration
sk_steam_client_t* sk_steam_client_create_with_config(sk_steam_configuration_t* config);

// Destroys a SteamClient
void sk_steam_client_destroy(sk_steam_client_t* client);

// Connects to the Steam network
void sk_steam_client_connect(sk_steam_client_t* client);

// Disconnects from the Steam network
void sk_steam_client_disconnect(sk_steam_client_t* client, bool user_initiated);

// Gets the next available JobID
sk_job_id_t* sk_steam_client_get_next_job_id(sk_steam_client_t* client);

// Checks if the client is connected
bool sk_steam_client_is_connected(const sk_steam_client_t* client);

// Registers a handler
void sk_steam_client_add_handler(struct sk_steam_client* client, struct sk_client_msg_handler* handler);

// Gets a registered handler by type (returns NULL if not found)
void* sk_steam_client_get_handler(const sk_steam_client_t* client, int handler_type);

// Convenience getter for unified messages handler
typedef struct sk_steam_unified_messages sk_steam_unified_messages_t;
sk_steam_unified_messages_t* sk_steam_client_get_unified_messages(const sk_steam_client_t* client);

// Dispatches a packet message to all handlers
void sk_steam_client_dispatch_msg(struct sk_steam_client* client, const sk_packet_msg_t* packet_msg);

// Sends a packet message to the network
void sk_steam_client_send(sk_steam_client_t* client, sk_packet_msg_t* packet_msg);

// Logs on to Steam
void sk_steam_client_log_on(sk_steam_client_t* client, const char* username, const char* password);

// Logs off from Steam
void sk_steam_client_log_off(sk_steam_client_t* client);

// Posts a callback to the internal queue (used by handlers)
void sk_steam_client_post_callback(sk_steam_client_t* client, uint32_t callback_type, uint64_t job_id, void* data);

// Waits for the next callback of any type. Returns callback data or NULL on timeout.
// If out_type/out_job_id are non-NULL, they receive the callback type and job id.
void* sk_steam_client_get_next_callback(sk_steam_client_t* client, uint32_t* out_type, uint64_t* out_job_id, int timeout_ms);

// Waits for a callback matching a specific job id. Returns callback data or NULL on timeout.
void* sk_steam_client_wait_for_job(sk_steam_client_t* client, uint64_t job_id, int timeout_ms);

// Frees callback data returned by wait functions (caller must call this)
void sk_steam_client_free_callback_data(void* data);

#ifdef __cplusplus
}
#endif

#endif // STEAMKIT_STEAM_STEAM_CLIENT_H
