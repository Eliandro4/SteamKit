#include "steamkit/networking/connection.h"
#include "steamkit/networking/tcp_connection.h"
#include "steamkit/networking/websocket_connection.h"
#include <stdlib.h>
#include <string.h>

void sk_connection_connect(sk_connection_t* conn, const char* host, uint16_t port, int timeout_ms) {
    if (!conn || !conn->vtable) return;
    typedef void (*connect_fn)(sk_connection_t*, const char*, uint16_t, int);
    ((connect_fn)conn->vtable)(conn, host, port, timeout_ms);
}

void sk_connection_disconnect(sk_connection_t* conn, bool user_initiated) {
    if (!conn) return;
    if (conn->protocol == SK_PROTOCOL_TYPE_TCP) {
        sk_tcp_connection_disconnect((sk_tcp_connection_t*)conn, user_initiated);
    } else if (conn->protocol == SK_PROTOCOL_TYPE_WEBSOCKET) {
        sk_websocket_connection_disconnect((sk_websocket_connection_t*)conn, user_initiated);
    }
}

void sk_connection_send(sk_connection_t* conn, const uint8_t* data, size_t len) {
    if (!conn || !data || len == 0) return;
    if (conn->protocol == SK_PROTOCOL_TYPE_TCP) {
        sk_tcp_connection_send((sk_tcp_connection_t*)conn, data, len);
    } else if (conn->protocol == SK_PROTOCOL_TYPE_WEBSOCKET) {
        sk_websocket_connection_send((sk_websocket_connection_t*)conn, data, len);
    }
}

const char* sk_connection_get_local_ip(const sk_connection_t* conn) {
    (void)conn;
    return "127.0.0.1";
}

sk_protocol_type_t sk_connection_protocol_type(const sk_connection_t* conn) {
    return conn ? conn->protocol : SK_PROTOCOL_TYPE_TCP;
}

bool sk_connection_is_connected(const sk_connection_t* conn) {
    return conn ? conn->is_connected : false;
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

ssize_t sk_connection_recv(sk_connection_t* conn, uint8_t* buf, size_t buf_len, int timeout_ms) {
    if (!conn || !conn->vtable) return -1;
    typedef ssize_t (*recv_fn)(sk_connection_t*, uint8_t*, size_t, int);
    return ((recv_fn)conn->vtable)(conn, buf, buf_len, timeout_ms);
}
