#include "steamkit/networking/udp_connection.h"
#include <stdlib.h>

typedef struct sk_udp_connection {
    sk_connection_t base;
} sk_udp_connection_t;

sk_udp_connection_t* sk_udp_connection_create(void) {
    sk_udp_connection_t* udp = (sk_udp_connection_t*)calloc(1, sizeof(sk_udp_connection_t));
    if (udp) {
        udp->base.protocol = SK_PROTOCOL_TYPE_UDP;
        udp->base.impl = udp;
    }
    return udp;
}
