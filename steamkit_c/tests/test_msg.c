#include "steamkit/base/packet_base.h"
#include "steamkit/base/client_msg.h"
#include "steamkit/base/emsg.h"
#include "steamkit/base/msg_hdr.h"
#include "steamkit/utils/msg_util.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    sk_packet_msg_t* pkt = sk_packet_msg_create(SK_EMSG_CLIENT_PERSONA_STATE, false);
    assert(pkt);
    assert(sk_packet_msg_is_proto(pkt) == false);
    assert(sk_packet_msg_msg_type(pkt) == SK_EMSG_CLIENT_PERSONA_STATE);
    sk_packet_msg_set_job_ids(pkt, 123456ULL, 0ULL);
    assert(sk_packet_msg_target_job_id(pkt) == 123456ULL);
    assert(sk_packet_msg_source_job_id(pkt) == 0ULL);

    uint8_t payload[] = {0x01, 0x02, 0x03, 0x04};
    sk_packet_msg_set_data(pkt, payload, sizeof(payload));
    size_t data_len = 0;
    const uint8_t* data = sk_packet_msg_data(pkt, &data_len);
    assert(data_len == sizeof(payload));
    assert(memcmp(data, payload, sizeof(payload)) == 0);
    sk_packet_msg_destroy(pkt);

    sk_client_msg_t* msg = sk_client_msg_create(SK_EMSG_CLIENT_FRIENDS_LIST, false);
    assert(msg);
    assert(sk_client_msg_is_proto(msg) == false);
    assert(sk_client_msg_msg_type(msg) == SK_EMSG_CLIENT_FRIENDS_LIST);
    sk_client_msg_set_session_id(msg, 42);
    assert(sk_client_msg_session_id(msg) == 42);
    sk_client_msg_set_target_job_id(msg, 999ULL);
    sk_client_msg_set_source_job_id(msg, 111ULL);
    assert(sk_client_msg_target_job_id(msg) == 999ULL);
    assert(sk_client_msg_source_job_id(msg) == 111ULL);

    sk_steam_id_t sid;
    sid.steamid = 76561198123456789ULL;
    sk_client_msg_set_steam_id(msg, &sid);
    assert(sk_client_msg_steam_id(msg)->steamid == 76561198123456789ULL);

    uint8_t body[] = {0xDE, 0xAD, 0xBE, 0xEF};
    sk_client_msg_set_data(msg, body, sizeof(body));
    size_t body_len = 0;
    const uint8_t* body_ptr = sk_client_msg_body(msg, &body_len);
    assert(body_len == sizeof(body));
    assert(memcmp(body_ptr, body, sizeof(body)) == 0);

    size_t out_size = 0;
    uint8_t* serialized = sk_client_msg_serialize(msg, &out_size);
    assert(serialized);
    assert(out_size == sizeof(body));
    assert(memcmp(serialized, body, sizeof(body)) == 0);
    free(serialized);

    sk_client_msg_destroy(msg);

    msg = sk_client_msg_create(SK_EMSG_CLIENT_LOG_ON, false);
    sk_client_msg_set_session_id(msg, 1);
    sk_client_msg_set_data(msg, (const uint8_t*)"test", 4);
    sk_packet_msg_t* pkt2 = sk_packet_msg_create_from_client_msg(msg);
    assert(pkt2);
    assert(sk_packet_msg_msg_type(pkt2) == SK_EMSG_CLIENT_LOG_ON);
    sk_client_msg_destroy(msg);
    sk_packet_msg_destroy(pkt2);

    sk_msg_hdr_t hdr;
    hdr.msg = SK_EMSG_CLIENT_PERSONA_STATE;
    hdr.target_job_id.base.value = 42ULL;
    hdr.source_job_id.base.value = 0ULL;
    uint8_t hdr_buf[20];
    size_t hdr_len = sk_msg_hdr_serialize(&hdr, hdr_buf, sizeof(hdr_buf));
    assert(hdr_len == 20);

    sk_packet_msg_t* pkt3 = sk_packet_msg_create_from_buffer(hdr_buf, hdr_len);
    assert(pkt3);
    assert(sk_packet_msg_msg_type(pkt3) == SK_EMSG_CLIENT_PERSONA_STATE);
    assert(sk_packet_msg_target_job_id(pkt3) == 42ULL);
    sk_packet_msg_destroy(pkt3);

    assert(sk_emsg_to_string(SK_EMSG_CLIENT_PERSONA_STATE) != NULL);
    assert(sk_emsg_to_string(SK_EMSG_INVALID) != NULL);
    assert(strcmp(sk_emsg_to_string(SK_EMSG_CLIENT_PERSONA_STATE), "ClientPersonaState") == 0);
    assert(sk_emsg_is_proto(SK_EMSG_CLIENT_TO_GC) == false);
    assert(sk_emsg_is_client_to_gc(SK_EMSG_CLIENT_TO_GC) == true);
    assert(sk_emsg_is_client_to_server(SK_EMSG_CLIENT_PERSONA_STATE) == true);
    assert(sk_emsg_is_server_to_client(SK_EMSG_CLIENT_FROM_GC) == true);
    assert(sk_emsg_is_server_to_client(SK_EMSG_CLIENT_HEARTBEAT) == false);

    {
        uint8_t proto[] = {
            0x08, 0x01,
            0x10, 0x02,
            0x1A, 0x03, 0x01, 0x02, 0x03
        };
        uint8_t wt;
        size_t offset;
        uint64_t value;
        size_t flen = sk_msg_find_field(proto, sizeof(proto), 1, &wt, &offset, &value);
        assert(flen == 1);
        assert(wt == 0);
        assert(value == 1);

        flen = sk_msg_find_field(proto, sizeof(proto), 2, &wt, &offset, &value);
        assert(flen == 1);
        assert(wt == 0);
        assert(value == 2);

        flen = sk_msg_find_field(proto, sizeof(proto), 3, &wt, &offset, &value);
        assert(flen == 4);
        assert(wt == 2);
    }

    printf("packet_msg and client_msg tests passed\n");

    sk_client_msg_t* proto_msg = sk_client_msg_create_proto(SK_EMSG_CLIENT_FRIENDS_LIST);
    assert(proto_msg);
    assert(sk_client_msg_is_proto(proto_msg) == true);
    sk_client_msg_set_header(proto_msg, SK_EMSG_CLIENT_FRIENDS_LIST, 7, 888ULL, 999ULL);
    assert(sk_client_msg_session_id(proto_msg) == 7);
    assert(sk_client_msg_target_job_id(proto_msg) == 888ULL);
    assert(sk_client_msg_source_job_id(proto_msg) == 999ULL);
    sk_client_msg_destroy(proto_msg);

    sk_packet_msg_t* pkt_type = sk_packet_msg_create(SK_EMSG_CLIENT_PERSONA_STATE, false);
    sk_packet_msg_set_msg_type(pkt_type, SK_EMSG_CLIENT_FRIENDS_LIST);
    assert(sk_packet_msg_msg_type(pkt_type) == SK_EMSG_CLIENT_FRIENDS_LIST);
    sk_packet_msg_destroy(pkt_type);

    // assert(sk_emsg_to_string(SK_EMSG_SERVER_FRIENDS_LIST) != NULL);
    // assert(strcmp(sk_emsg_to_string(SK_EMSG_SERVER_FRIENDS_LIST), "ServerFriendsList") == 0);
    // assert(sk_emsg_is_server_to_client(SK_EMSG_SERVER_FRIENDS_LIST) == true);
    // assert(sk_emsg_is_server_to_client(SK_EMSG_SERVER_LOBBY_INVITE) == true);
    assert(sk_emsg_is_server_to_client(SK_EMSG_CLIENT_LOG_ON) == false);

    return 0;
}

