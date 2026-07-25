#ifndef STEAMKIT_NETWORKING_UDP_CONNECTION_H
#define STEAMKIT_NETWORKING_UDP_CONNECTION_H

#include "steamkit/networking/connection.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sk_udp_connection sk_udp_connection_t;

sk_udp_connection_t* sk_udp_connection_create(void);

#ifdef __cplusplus
}
#endif

#endif // STEAMKIT_NETWORKING_UDP_CONNECTION_H
