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

// Connects to a WebSocket server. Returns true on success.
// When SK_ENABLE_CURL is not defined, always returns false.
bool sk_websocket_connection_connect(sk_websocket_connection_t* ws, const char* url, int timeout_ms);

// Disconnects from the WebSocket server
void sk_websocket_connection_disconnect(sk_websocket_connection_t* ws, bool user_initiated);

// Sends data over the WebSocket
void sk_websocket_connection_send(sk_websocket_connection_t* ws, const uint8_t* data, size_t len);

// Receives data from the WebSocket
ssize_t sk_websocket_connection_recv(sk_websocket_connection_t* ws, uint8_t* buf, size_t buf_len, int timeout_ms);

#ifdef __cplusplus
}
#endif

#endif // STEAMKIT_NETWORKING_WEBSOCKET_CONNECTION_H
