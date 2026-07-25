#include "steamkit/networking/connection.h"
#include <stdlib.h>
#include <string.h>

void sk_connection_connect(sk_connection_t* conn, const char* host, uint16_t port, int timeout_ms) {
    if (!conn || !conn->vtable) return;
    typedef void (*connect_fn)(sk_connection_t*, const char*, uint16_t, int);
    ((connect_fn)conn->vtable)(conn, host, port, timeout_ms);
}

void sk_connection_disconnect(sk_connection_t* conn, bool user_initiated) {
    if (!conn || !conn->vtable) return;
    typedef void (*disconnect_fn)(sk_connection_t*, bool);
    ((disconnect_fn)conn->vtable)(conn, user_initiated);
}

void sk_connection_send(sk_connection_t* conn, const uint8_t* data, size_t len) {
    if (!conn || !conn->vtable) return;
    typedef void (*send_fn)(sk_connection_t*, const uint8_t*, size_t);
    ((send_fn)conn->vtable)(conn, data, len);
}

const char* sk_connection_get_local_ip(const sk_connection_t* conn) {
    (void)conn;
    return "127.0.0.1";
}

sk_protocol_type_t sk_connection_protocol_type(const sk_connection_t* conn) {
    return conn ? conn->protocol : SK_PROTOCOL_TYPE_TCP;
}

bool sk_connection_is_connected(const sk_connection_t* conn) {
    (void)conn;
    return false;
}

void sk_connection_set_callbacks(sk_connection_t* conn,
                                 sk_connection_net_msg_fn net_msg_fn,
                                 sk_connection_connected_fn connected_fn,
                                 sk_connection_disconnected_fn disconnected_fn) {
    if (conn) {
        conn->net_msg_callback = net_msg_fn;
        conn->connected_callback = connected_fn;
        conn->disconnected_callback = disconnected_fn;
    }
}

void sk_connection_set_user_data(sk_connection_t* conn, void* user_data) {
    if (conn) conn->user_data = user_data;
}

void* sk_connection_get_user_data(const sk_connection_t* conn) {
    return conn ? conn->user_data : NULL;
}

void sk_connection_destroy(sk_connection_t* conn) {
    free(conn);
}
