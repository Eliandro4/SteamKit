#include "steamkit/steam/handlers/client_msg_handler.h"
#include <stdlib.h>

void sk_client_msg_handler_handle_msg(struct sk_client_msg_handler* handler, const sk_packet_msg_t* packet_msg) {
    if (handler && handler->handle_msg) {
        handler->handle_msg(handler, packet_msg);
    }
}

void sk_client_msg_handler_setup(struct sk_client_msg_handler* handler, struct sk_steam_client* client) {
    if (handler) {
        handler->client = client;
    }
}

bool sk_client_msg_handler_get_expect_disconnection(const struct sk_client_msg_handler* handler) {
    return handler ? handler->expect_disconnection : false;
}

void sk_client_msg_handler_set_expect_disconnection(struct sk_client_msg_handler* handler, bool expect) {
    if (handler) {
        handler->expect_disconnection = expect;
    }
}

void sk_client_msg_handler_destroy(struct sk_client_msg_handler* handler) {
    free(handler);
}
