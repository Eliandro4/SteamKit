#ifndef STEAMKIT_NETWORKING_CONNECTION_H
#define STEAMKIT_NETWORKING_CONNECTION_H

#include <stdint.h>
#include <stdbool.h>
#include "steamkit/base/emsg.h"
#include "steamkit/base/packet_base.h"

#ifdef __cplusplus
extern "C" {
#endif

// Callback types for connection events
typedef void (*sk_connection_net_msg_fn)(void* user_data, const uint8_t* data, size_t len, sk_emsg_t msg);
typedef void (*sk_connection_connected_fn)(void* user_data);
typedef void (*sk_connection_disconnected_fn)(void* user_data, bool user_initiated);

typedef struct sk_connection {
    sk_protocol_type_t protocol;
    // Virtual function table for polymorphic behavior
    void* vtable;
    void* impl;
    void* user_data;
    sk_connection_net_msg_fn net_msg_callback;
    sk_connection_connected_fn connected_callback;
    sk_connection_disconnected_fn disconnected_callback;
} sk_connection_t;

// Connection interface functions
void sk_connection_connect(sk_connection_t* conn, const char* host, uint16_t port, int timeout_ms);
void sk_connection_disconnect(sk_connection_t* conn, bool user_initiated);
void sk_connection_send(sk_connection_t* conn, const uint8_t* data, size_t len);
const char* sk_connection_get_local_ip(const sk_connection_t* conn);
sk_protocol_type_t sk_connection_protocol_type(const sk_connection_t* conn);
bool sk_connection_is_connected(const sk_connection_t* conn);
void sk_connection_set_user_data(sk_connection_t* conn, void* user_data);
void* sk_connection_get_user_data(const sk_connection_t* conn);
void sk_connection_set_callbacks(sk_connection_t* conn, 
                                 sk_connection_net_msg_fn net_msg_fn,
                                 sk_connection_connected_fn connected_fn,
                                 sk_connection_disconnected_fn disconnected_fn);
void sk_connection_set_user_data(sk_connection_t* conn, void* user_data);
void sk_connection_destroy(sk_connection_t* conn);

#ifdef __cplusplus
}
#endif

#endif // STEAMKIT_NETWORKING_CONNECTION_H
