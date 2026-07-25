#ifndef STEAMKIT_NETWORKING_WEBSOCKET_CONNECTION_H
#define STEAMKIT_NETWORKING_WEBSOCKET_CONNECTION_H

#include "steamkit/networking/connection.h"

#ifdef __cplusplus
extern "C" {
#endif

// WebSocket Connection - mirrors C# WebSocketConnection
typedef struct sk_websocket_connection sk_websocket_connection_t;

typedef struct {
    const char* url;
    const char** subprotocols;
    size_t subprotocol_count;
    const char* user_agent;
} sk_websocket_context_t;

// Creates a new WebSocket connection
sk_websocket_connection_t* sk_websocket_connection_create(const sk_websocket_context_t* context);

// Destroys a WebSocket connection
void sk_websocket_connection_destroy(sk_websocket_connection_t* ws);

#ifdef __cplusplus
}
#endif

#endif // STEAMKIT_NETWORKING_WEBSOCKET_CONNECTION_H
