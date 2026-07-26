#ifndef STEAMKIT_NETWORKING_TCP_CONNECTION_H
#define STEAMKIT_NETWORKING_TCP_CONNECTION_H

#include <sys/types.h>
#include "steamkit/networking/connection.h"

#ifdef __cplusplus
extern "C" {
#endif

// TCP Connection - mirrors C# TcpConnection
typedef struct sk_tcp_connection sk_tcp_connection_t;

// Creates a new TCP connection
sk_tcp_connection_t* sk_tcp_connection_create(void);

// Destroys a TCP connection
void sk_tcp_connection_destroy(sk_tcp_connection_t* tcp);

// Connects to a remote host
void sk_tcp_connection_connect(sk_tcp_connection_t* tcp, const char* host, uint16_t port, int timeout_ms);

// Disconnects from the remote host
void sk_tcp_connection_disconnect(sk_tcp_connection_t* tcp, bool user_initiated);

// Sends data to the remote host
void sk_tcp_connection_send(sk_tcp_connection_t* tcp, const uint8_t* data, size_t len);

// Receives data from the remote host
ssize_t sk_tcp_connection_recv(sk_tcp_connection_t* tcp, uint8_t* buf, size_t buf_len, int timeout_ms);

#ifdef __cplusplus
}
#endif

#endif // STEAMKIT_NETWORKING_TCP_CONNECTION_H
