#ifndef STEAMKIT_NETWORKING_UDP_CONNECTION_H
#define STEAMKIT_NETWORKING_UDP_CONNECTION_H

#include "steamkit/networking/connection.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sk_udp_connection sk_udp_connection_t;

sk_udp_connection_t* sk_udp_connection_create(void);

void sk_udp_connection_connect(sk_udp_connection_t* udp, const char* host, uint16_t port, int timeout_ms);
void sk_udp_connection_disconnect(sk_udp_connection_t* udp, bool user_initiated);
void sk_udp_connection_send(sk_udp_connection_t* udp, const uint8_t* data, size_t len);
void sk_udp_connection_bind(sk_udp_connection_t* udp, uint16_t port);
ssize_t sk_udp_connection_recv(sk_udp_connection_t* udp, uint8_t* buf, size_t buf_len, int timeout_ms);

#ifdef __cplusplus
}
#endif

#endif // STEAMKIT_NETWORKING_UDP_CONNECTION_H
