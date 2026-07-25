#ifndef STEAMKIT_BASE_CLIENT_MSG_H
#define STEAMKIT_BASE_CLIENT_MSG_H

#include <stdint.h>
#include <stdbool.h>
#include "steamkit/base/emsg.h"
#include "steamkit/base/msg_hdr.h"
#include "steamkit/base/packet_base.h"
#include "steamkit/types/steam_id.h"
#include "steamkit/types/job_id.h"

#ifdef __cplusplus
extern "C" {
#endif

// Client message interface - mirrors C# IClientMsg
typedef struct sk_client_msg sk_client_msg_t;

struct sk_client_msg {
    bool is_proto;
    sk_emsg_t msg_type;
    int session_id;
    void* vtable;
    void* impl;
    uint8_t* body;
    size_t body_len;
    sk_steam_id_t steam_id;
    uint64_t target_job_id;
    uint64_t source_job_id;
};

// Client message functions
sk_client_msg_t* sk_client_msg_create(sk_emsg_t msg_type, bool is_proto);
sk_client_msg_t* sk_client_msg_create_with_body(sk_emsg_t msg_type, bool is_proto, const uint8_t* body, size_t body_len);
sk_client_msg_t* sk_client_msg_create_proto(sk_emsg_t msg_type);
void sk_client_msg_destroy(sk_client_msg_t* msg);
void sk_client_msg_set_header(sk_client_msg_t* msg, sk_emsg_t msg_type, int session_id, uint64_t target_job_id, uint64_t source_job_id);
void sk_client_msg_set_data(sk_client_msg_t* msg, const uint8_t* data, size_t len);
const uint8_t* sk_client_msg_body(const sk_client_msg_t* msg, size_t* out_len);

bool sk_client_msg_is_proto(const sk_client_msg_t* msg);
sk_emsg_t sk_client_msg_msg_type(const sk_client_msg_t* msg);
int sk_client_msg_session_id(const sk_client_msg_t* msg);
void sk_client_msg_set_session_id(sk_client_msg_t* msg, int session_id);
const sk_steam_id_t* sk_client_msg_steam_id(const sk_client_msg_t* msg);
void sk_client_msg_set_steam_id(sk_client_msg_t* msg, const sk_steam_id_t* steam_id);
uint64_t sk_client_msg_target_job_id(const sk_client_msg_t* msg);
void sk_client_msg_set_target_job_id(sk_client_msg_t* msg, uint64_t job_id);
uint64_t sk_client_msg_source_job_id(const sk_client_msg_t* msg);
void sk_client_msg_set_source_job_id(sk_client_msg_t* msg, uint64_t job_id);
uint8_t* sk_client_msg_serialize(const sk_client_msg_t* msg, size_t* out_size);
void sk_client_msg_destroy(sk_client_msg_t* msg);

#ifdef __cplusplus
}
#endif

#endif // STEAMKIT_BASE_CLIENT_MSG_H
