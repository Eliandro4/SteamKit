#ifndef STEAMKIT_UTILS_NET_HELPERS_H
#define STEAMKIT_UTILS_NET_HELPERS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Generates a random uint32
uint32_t sk_net_random_uint32(void);

// Generates a random uint64
uint64_t sk_net_random_uint64(void);

// Gets the local IPv4 address
uint32_t sk_net_get_local_ip(void);

// Creates a UDP socket
int sk_net_create_udp_socket(void);

// Closes a socket
void sk_net_close_socket(int socket);

#ifdef __cplusplus
}
#endif

#endif // STEAMKIT_UTILS_NET_HELPERS_H
