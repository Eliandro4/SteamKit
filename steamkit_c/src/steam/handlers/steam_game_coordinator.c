#include "steamkit/steam/handlers/steam_game_coordinator.h"
#include "steamkit/steam/handlers/client_msg_handler.h"
#include "steamkit/steam/handlers/client_msg_protobuf.h"
#include "steamkit/steam/steam_client.h"
#include "steamkit/utils/debug_log.h"
#include "steammessages_base.pb-c.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

// Internal packet layout for CMsgGCClient (mirrors SteamKit2 GC)
// The GC payload is wrapped in a CMsgGCClient protobuf sent via EMsg.ClientToGC

static void sk_steam_gc_handle_msg(struct sk_client_msg_handler* handler, const sk_packet_msg_t* packet_msg) {
    if (!handler || !packet_msg) return;

    uint32_t msg_type = sk_packet_msg_msg_type(packet_msg);

    if ((sk_emsg_t)msg_type == SK_EMSG_CLIENT_FROM_GC) {
        sk_debug_log_info("SteamGC", "Received message from Game Coordinator");
        // TODO: parse CMsgGCClient and dispatch via callback
    }
}

typedef struct sk_steam_game_coordinator {
    struct sk_client_msg_handler base;
} sk_steam_game_coordinator_t;

sk_steam_game_coordinator_t* sk_steam_game_coordinator_create(void) {
    sk_steam_game_coordinator_t* gc = (sk_steam_game_coordinator_t*)calloc(1, sizeof(sk_steam_game_coordinator_t));
    if (gc) {
        gc->base.handle_msg = sk_steam_gc_handle_msg;
    }
    return gc;
}

void sk_steam_game_coordinator_destroy(sk_steam_game_coordinator_t* gc) {
    free(gc);
}

void sk_steam_game_coordinator_send(sk_steam_game_coordinator_t* gc, uint32_t app_id,
                                     const uint8_t* payload, size_t payload_len,
                                     uint32_t gc_msg_type) {
    if (!gc || !gc->base.client || !payload || payload_len == 0) return;

    // Mirror SteamKit2: wrap the GC message in a ClientMsgProtobuf<CMsgGCClient>(EMsg.ClientToGC)
    sk_client_msg_protobuf_t* msg = sk_client_msg_protobuf_create(SK_EMSG_CLIENT_TO_GC);
    if (!msg) return;

    // Set routing appid in the proto header
    CMsgProtoBufHeader* hdr = sk_client_msg_protobuf_header(msg);
    if (hdr) {
        hdr->routing_appid = app_id;
        hdr->has_routing_appid = 1;
    }

    // Build CMsgGCClient body (msgtype + appid + payload)
    // Layout (little-endian binary, matching SteamKit2 MsgUtil.MakeGCMsg):
    //   [4 bytes] msgtype (with proto flag if needed)
    //   [4 bytes] appid
    //   [N bytes] payload
    size_t body_len = 4 + 4 + payload_len;
    uint8_t* body = (uint8_t*)malloc(body_len);
    if (!body) {
        sk_client_msg_protobuf_destroy(msg);
        return;
    }

    body[0] = gc_msg_type & 0xFF;
    body[1] = (gc_msg_type >> 8) & 0xFF;
    body[2] = (gc_msg_type >> 16) & 0xFF;
    body[3] = (gc_msg_type >> 24) & 0xFF;
    body[4] = app_id & 0xFF;
    body[5] = (app_id >> 8) & 0xFF;
    body[6] = (app_id >> 16) & 0xFF;
    body[7] = (app_id >> 24) & 0xFF;
    memcpy(body + 8, payload, payload_len);

    sk_client_msg_protobuf_set_body(msg, body, body_len);
    free(body);

    sk_packet_msg_t* pkt = sk_packet_msg_create_from_client_msg_protobuf(msg);
    if (pkt) {
        sk_steam_client_send(gc->base.client, pkt);
        sk_packet_msg_destroy(pkt);
    }
    sk_client_msg_protobuf_destroy(msg);

    sk_debug_log_info("SteamGC", "Sent GC message type %u to app %u (%zu bytes)", gc_msg_type, app_id, payload_len);
}
