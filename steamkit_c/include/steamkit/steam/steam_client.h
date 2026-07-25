#ifndef STEAMKIT_STEAM_STEAM_CLIENT_H
#define STEAMKIT_STEAM_STEAM_CLIENT_H

#include <stdint.h>
#include <stdbool.h>
#include "steamkit/steam/cm_client.h"
#include "steamkit/base/client_msg.h"
#include "steamkit/types/job_id.h"

// Forward declarations needed for handler interface
struct sk_client_msg_handler;
struct sk_steam_client;

#ifdef __cplusplus
extern "C" {
#endif

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

// Dispatches a packet message to all handlers
void sk_steam_client_dispatch_msg(struct sk_steam_client* client, const sk_packet_msg_t* packet_msg);

// Sends a packet message to the network
void sk_steam_client_send(sk_steam_client_t* client, sk_packet_msg_t* packet_msg);

// Logs on to Steam
void sk_steam_client_log_on(sk_steam_client_t* client, const char* username, const char* password);

// Logs off from Steam
void sk_steam_client_log_off(sk_steam_client_t* client);

#ifdef __cplusplus
}
#endif

#endif // STEAMKIT_STEAM_STEAM_CLIENT_H
