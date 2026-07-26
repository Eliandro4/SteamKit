#include "steamkit/steam/handlers/steam_networking.h"
#include "steamkit/steam/handlers/client_msg_handler.h"
#include "steamkit/steam/steam_client.h"
#include "steamkit/steam/callbacks.h"
#include "steamkit/utils/debug_log.h"
#include "steamkit/steam/handlers/client_msg_protobuf.h"
#include "steammessages_clientserver.pb-c.h"
#include "steammessages_gamenetworking.steamclient.pb-c.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct sk_steam_networking {
    struct sk_client_msg_handler base;
} sk_steam_networking_t;

static void sk_steam_net_handle_msg(struct sk_client_msg_handler* handler, const sk_packet_msg_t* packet_msg) {
    if (!handler || !packet_msg) return;
    (void)handler;
    (void)packet_msg;
    sk_debug_log_info("SteamNetworking", "Received message");
}

sk_steam_networking_t* sk_steam_networking_create(void) {
    sk_steam_networking_t* networking = (sk_steam_networking_t*)calloc(1, sizeof(sk_steam_networking_t));
    if (networking) {
        networking->base.handle_msg = sk_steam_net_handle_msg;
        networking->base.handler_type = SK_HANDLER_STEAM_NETWORKING;
    }
    return networking;
}

void sk_steam_networking_destroy(sk_steam_networking_t* net) {
    free(net);
}

void sk_steam_networking_send_net_message(sk_steam_networking_t* net, uint32_t msg_type, const uint8_t* data, size_t len) {
    if (!net || !net->base.client) return;

    sk_client_msg_protobuf_t* msg = sk_client_msg_protobuf_create(msg_type);
    if (!msg) return;

    if (data && len > 0) {
        uint8_t* buf = (uint8_t*)malloc(len);
        if (buf) {
            memcpy(buf, data, len);
            sk_client_msg_protobuf_set_body(msg, buf, len);
            free(buf);
        }
    }

    sk_packet_msg_t* pkt = sk_packet_msg_create_from_client_msg_protobuf(msg);
    if (pkt) {
        sk_steam_client_send(net->base.client, pkt);
        sk_packet_msg_destroy(pkt);
    }

    sk_client_msg_protobuf_destroy(msg);
    sk_debug_log_info("SteamNetworking", "Sent net message type=%u len=%zu", msg_type, len);
}