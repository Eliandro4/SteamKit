#include "steamkit/steam/handlers/steam_user_stats.h"
#include "steamkit/steam/handlers/client_msg_handler.h"
#include "steamkit/steam/handlers/client_msg_protobuf.h"
#include "steamkit/steam/steam_client.h"
#include "steamkit/utils/debug_log.h"
#include "steammessages_clientserver_lbs.pb-c.h"
#include "steammessages_clientserver_2.pb-c.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

static void sk_steam_stats_handle_msg(struct sk_client_msg_handler* handler, const sk_packet_msg_t* packet_msg) {
    (void)handler;
    if (!packet_msg) return;

    uint32_t msg_type = sk_packet_msg_msg_type(packet_msg);
    switch ((sk_emsg_t)msg_type) {
        case SK_EMSG_CLIENT_GET_NUMBER_OF_CURRENT_PLAYERS_DP_RESPONSE: {
            sk_debug_log_info("SteamUserStats", "Received current players response");
            break;
        }
        case SK_EMSG_CLIENT_LBS_FIND_OR_CREATE_LB_RESPONSE: {
            sk_debug_log_info("SteamUserStats", "Received leaderboard find/create response");
            break;
        }
        case SK_EMSG_CLIENT_LBS_GET_LB_ENTRIES_RESPONSE: {
            sk_debug_log_info("SteamUserStats", "Received leaderboard entries response");
            break;
        }
        default:
            break;
    }
}

typedef struct sk_steam_user_stats {
    struct sk_client_msg_handler base;
} sk_steam_user_stats_t;

sk_steam_user_stats_t* sk_steam_user_stats_create(void) {
    sk_steam_user_stats_t* stats = (sk_steam_user_stats_t*)calloc(1, sizeof(sk_steam_user_stats_t));
    if (stats) {
        stats->base.handle_msg = sk_steam_stats_handle_msg;
        stats->base.handler_type = SK_HANDLER_STEAM_USER_STATS;
    }
    return stats;
}

void sk_steam_user_stats_destroy(sk_steam_user_stats_t* stats) {
    free(stats);
}

// Mirrors SteamKit2: SteamUserStats.GetNumberOfCurrentPlayers(appId)
void sk_steam_user_stats_get_number_of_current_players(sk_steam_user_stats_t* stats, uint32_t app_id) {
    if (!stats || !stats->base.client) return;

    sk_client_msg_protobuf_t* msg = sk_client_msg_protobuf_create(SK_EMSG_CLIENT_GET_NUMBER_OF_CURRENT_PLAYERS_DP);
    if (!msg) return;

    CMsgDPGetNumberOfCurrentPlayers req = CMSG_DPGET_NUMBER_OF_CURRENT_PLAYERS__INIT;
    req.appid = app_id;
    req.has_appid = true;

    size_t packed_size = cmsg_dpget_number_of_current_players__get_packed_size(&req);
    uint8_t* packed_buf = (uint8_t*)malloc(packed_size);
    if (packed_buf) {
        cmsg_dpget_number_of_current_players__pack(&req, packed_buf);
        sk_client_msg_protobuf_set_body(msg, packed_buf, packed_size);
        free(packed_buf);
    }

    sk_packet_msg_t* pkt = sk_packet_msg_create_from_client_msg_protobuf(msg);
    if (pkt) {
        sk_steam_client_send(stats->base.client, pkt);
        sk_packet_msg_destroy(pkt);
    }
    sk_client_msg_protobuf_destroy(msg);
    sk_debug_log_info("SteamUserStats", "Requested number of current players for app: %u", app_id);
}

// Mirrors SteamKit2: SteamUserStats.FindLeaderboard(appId, name)
// routing_appid must be set in the proto header (SteamKit2 does this)
void sk_steam_user_stats_find_leaderboard(sk_steam_user_stats_t* stats, uint32_t app_id, const char* name) {
    if (!stats || !stats->base.client || !name) return;

    sk_client_msg_protobuf_t* msg = sk_client_msg_protobuf_create(SK_EMSG_CLIENT_LBS_FIND_OR_CREATE_LB);
    if (!msg) return;

    // Set routing_appid in header — required for Steam to route response back correctly
    CMsgProtoBufHeader* hdr = sk_client_msg_protobuf_header(msg);
    if (hdr) {
        hdr->routing_appid = app_id;
        hdr->has_routing_appid = 1;
    }

    CMsgClientLBSFindOrCreateLB req = CMSG_CLIENT_LBSFIND_OR_CREATE_LB__INIT;
    req.app_id = app_id;
    req.has_app_id = true;
    req.leaderboard_name = (char*)name;
    req.create_if_not_found = false;
    req.has_create_if_not_found = true;

    size_t packed_size = cmsg_client_lbsfind_or_create_lb__get_packed_size(&req);
    uint8_t* packed_buf = (uint8_t*)malloc(packed_size);
    if (packed_buf) {
        cmsg_client_lbsfind_or_create_lb__pack(&req, packed_buf);
        sk_client_msg_protobuf_set_body(msg, packed_buf, packed_size);
        free(packed_buf);
    }

    sk_packet_msg_t* pkt = sk_packet_msg_create_from_client_msg_protobuf(msg);
    if (pkt) {
        sk_steam_client_send(stats->base.client, pkt);
        sk_packet_msg_destroy(pkt);
    }
    sk_client_msg_protobuf_destroy(msg);
    sk_debug_log_info("SteamUserStats", "Requested leaderboard '%s' for app: %u", name, app_id);
}

// Mirrors SteamKit2: SteamUserStats.GetLeaderboardEntries(appId, id, rangeStart, rangeEnd, dataRequest)
void sk_steam_user_stats_get_leaderboard_entries(sk_steam_user_stats_t* stats, uint32_t app_id,
                                                   int32_t leaderboard_id, int32_t range_start,
                                                   int32_t range_end, int32_t data_request) {
    if (!stats || !stats->base.client) return;

    sk_client_msg_protobuf_t* msg = sk_client_msg_protobuf_create(SK_EMSG_CLIENT_LBS_GET_LB_ENTRIES);
    if (!msg) return;

    // Set routing_appid
    CMsgProtoBufHeader* hdr = sk_client_msg_protobuf_header(msg);
    if (hdr) {
        hdr->routing_appid = app_id;
        hdr->has_routing_appid = 1;
    }

    CMsgClientLBSGetLBEntries req = CMSG_CLIENT_LBSGET_LBENTRIES__INIT;
    req.app_id = (int32_t)app_id;
    req.has_app_id = true;
    req.leaderboard_id = leaderboard_id;
    req.has_leaderboard_id = true;
    req.range_start = range_start;
    req.has_range_start = true;
    req.range_end = range_end;
    req.has_range_end = true;
    req.leaderboard_data_request = data_request;
    req.has_leaderboard_data_request = true;

    size_t packed_size = cmsg_client_lbsget_lbentries__get_packed_size(&req);
    uint8_t* packed_buf = (uint8_t*)malloc(packed_size);
    if (packed_buf) {
        cmsg_client_lbsget_lbentries__pack(&req, packed_buf);
        sk_client_msg_protobuf_set_body(msg, packed_buf, packed_size);
        free(packed_buf);
    }

    sk_packet_msg_t* pkt = sk_packet_msg_create_from_client_msg_protobuf(msg);
    if (pkt) {
        sk_steam_client_send(stats->base.client, pkt);
        sk_packet_msg_destroy(pkt);
    }
    sk_client_msg_protobuf_destroy(msg);
    sk_debug_log_info("SteamUserStats", "Requested leaderboard entries for leaderboard %d, app: %u",
                      leaderboard_id, app_id);
}
