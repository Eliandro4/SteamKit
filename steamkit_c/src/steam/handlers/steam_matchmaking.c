#include "steamkit/steam/handlers/steam_matchmaking.h"
#include "steamkit/steam/handlers/client_msg_handler.h"
#include "steamkit/steam/handlers/client_msg_protobuf.h"
#include "steamkit/steam/steam_client.h"
#include "steamkit/steam/callbacks.h"
#include "steamkit/utils/debug_log.h"
#include "steammessages_clientserver_mms.pb-c.h"
#include <stdlib.h>
#include <string.h>

static char* sk_strdup(const char* s) {
    if (!s) return NULL;
    size_t len = strlen(s) + 1;
    char* dup = (char*)malloc(len);
    if (dup) memcpy(dup, s, len);
    return dup;
}

typedef struct sk_steam_matchmaking {
    struct sk_client_msg_handler base;
} sk_steam_matchmaking_t;

static void sk_steam_mm_handle_msg(struct sk_client_msg_handler* handler, const sk_packet_msg_t* packet_msg) {
    if (!handler || !packet_msg) return;

    uint32_t msg_type = sk_packet_msg_msg_type(packet_msg);
    sk_client_msg_protobuf_t* proto = sk_client_msg_protobuf_create_from_packet(packet_msg);
    if (!proto) return;

    CMsgProtoBufHeader* hdr = sk_client_msg_protobuf_header(proto);
    uint64_t job_id = hdr && hdr->has_jobid_target ? hdr->jobid_target : 0;

    switch ((sk_emsg_t)msg_type) {
        case SK_EMSG_CLIENT_MMS_GET_LOBBY_LIST_RESPONSE: {
            size_t data_len = 0;
            const uint8_t* data = sk_packet_msg_data(packet_msg, &data_len);
            CMsgClientMMSGetLobbyListResponse* resp = cmsg_client_mmsget_lobby_list_response__unpack(NULL, data_len, data);
            if (resp) {
                uint64_t* lobby_steam_ids = NULL;
                int32_t* lobby_types = NULL;
                int32_t* distances = NULL;
                if (resp->lobbies && resp->n_lobbies > 0) {
                    lobby_steam_ids = (uint64_t*)malloc(resp->n_lobbies * sizeof(uint64_t));
                    lobby_types = (int32_t*)malloc(resp->n_lobbies * sizeof(int32_t));
                    distances = (int32_t*)malloc(resp->n_lobbies * sizeof(int32_t));
                    if (lobby_steam_ids && lobby_types && distances) {
                        for (size_t i = 0; i < resp->n_lobbies; ++i) {
                            lobby_steam_ids[i] = resp->lobbies[i]->has_steam_id ? resp->lobbies[i]->steam_id : 0;
                            lobby_types[i] = resp->lobbies[i]->has_lobby_type ? resp->lobbies[i]->lobby_type : 0;
                            distances[i] = resp->lobbies[i]->has_distance ? (int32_t)resp->lobbies[i]->distance : 0;
                        }
                    } else {
                        free(lobby_steam_ids);
                        free(lobby_types);
                        free(distances);
                        lobby_steam_ids = NULL;
                        lobby_types = NULL;
                        distances = NULL;
                    }
                }
                sk_lobby_matchmaking_callback_t* cb = sk_lobby_matchmaking_callback_create(resp->eresult, lobby_steam_ids, (uint32_t)resp->n_lobbies, lobby_types, distances);
                free(lobby_steam_ids);
                free(lobby_types);
                free(distances);
                if (handler->client) {
                    sk_steam_client_post_callback(handler->client, SK_CLIENT_CALLBACK_LOBBY_MATCHMAKING, job_id, cb);
                }
                sk_lobby_matchmaking_callback_destroy(cb);
                cmsg_client_mmsget_lobby_list_response__free_unpacked(resp, NULL);
            }
            break;
        }
        default:
            break;
    }

    sk_client_msg_protobuf_destroy(proto);
}

sk_steam_matchmaking_t* sk_steam_matchmaking_create(void) {
    sk_steam_matchmaking_t* mm = (sk_steam_matchmaking_t*)calloc(1, sizeof(sk_steam_matchmaking_t));
    if (mm) {
        mm->base.handle_msg = sk_steam_mm_handle_msg;
        mm->base.handler_type = SK_HANDLER_STEAM_MATCHMAKING;
    }
    return mm;
}

void sk_steam_matchmaking_destroy(sk_steam_matchmaking_t* mm) {
    free(mm);
}

sk_lobby_matchmaking_callback_t* sk_steam_matchmaking_request_lobby_list(sk_steam_matchmaking_t* mm, uint32_t app_id, uint32_t num_lobbies_requested, uint32_t cell_id) {
    if (!mm || !mm->base.client) return NULL;

    sk_client_msg_protobuf_t* msg = sk_client_msg_protobuf_create(SK_EMSG_CLIENT_MMS_GET_LOBBY_LIST);
    if (!msg) return NULL;

    CMsgClientMMSGetLobbyList req = CMSG_CLIENT_MMSGET_LOBBY_LIST__INIT;
    req.app_id = app_id;
    req.has_app_id = true;
    req.num_lobbies_requested = (int32_t)num_lobbies_requested;
    req.has_num_lobbies_requested = true;
    req.cell_id = cell_id;
    req.has_cell_id = true;

    size_t packed_size = cmsg_client_mmsget_lobby_list__get_packed_size(&req);
    uint8_t* packed_buf = (uint8_t*)malloc(packed_size);
    if (packed_buf) {
        cmsg_client_mmsget_lobby_list__pack(&req, packed_buf);
        sk_client_msg_protobuf_set_body(msg, packed_buf, packed_size);
        free(packed_buf);
    }

    sk_packet_msg_t* pkt = sk_packet_msg_create_from_client_msg_protobuf(msg);
    if (!pkt) {
        sk_client_msg_protobuf_destroy(msg);
        return NULL;
    }

    CMsgProtoBufHeader* hdr = sk_client_msg_protobuf_header(msg);
    uint64_t job_id = hdr && hdr->has_jobid_target ? hdr->jobid_target : 0;
    if (job_id == 0) {
        sk_job_id_t* job = sk_steam_client_get_next_job_id(mm->base.client);
        if (job) {
            job_id = sk_job_id_value(job);
            sk_job_id_destroy(job);
        }
    }
    sk_packet_msg_set_job_ids(pkt, job_id, 0);

    sk_steam_client_send(mm->base.client, pkt);
    sk_packet_msg_destroy(pkt);
    sk_client_msg_protobuf_destroy(msg);

    sk_debug_log_info("SteamMatchmaking", "Requested lobby list for app %u (job %llu)", app_id, (unsigned long long)job_id);

    sk_lobby_matchmaking_callback_t* result = (sk_lobby_matchmaking_callback_t*)sk_steam_client_wait_for_job(mm->base.client, job_id, 30000);
    if (!result) {
        sk_debug_log_warn("SteamMatchmaking", "Lobby list request timed out");
    }
    return result;
}