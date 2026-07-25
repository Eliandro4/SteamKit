#include "steamkit/base/client_msg.h"
#include "steamkit/base/msg_hdr.h"
#include "steamkit/base/packet_base.h"
#include <stdlib.h>
#include <string.h>

sk_client_msg_t* sk_client_msg_create(sk_emsg_t msg_type, bool is_proto) {
    sk_client_msg_t* msg = (sk_client_msg_t*)calloc(1, sizeof(sk_client_msg_t));
    if (msg) {
        msg->msg_type = msg_type;
        msg->is_proto = is_proto;
    }
    return msg;
}

sk_client_msg_t* sk_client_msg_create_with_body(sk_emsg_t msg_type, bool is_proto, const uint8_t* body, size_t body_len) {
    sk_client_msg_t* msg = sk_client_msg_create(msg_type, is_proto);
    if (!msg) return NULL;
    sk_client_msg_set_data(msg, body, body_len);
    return msg;
}

sk_client_msg_t* sk_client_msg_create_proto(sk_emsg_t msg_type) {
    return sk_client_msg_create(msg_type, true);
}

void sk_client_msg_set_header(sk_client_msg_t* msg, sk_emsg_t msg_type, int session_id, uint64_t target_job_id, uint64_t source_job_id) {
    if (!msg) return;
    msg->msg_type = msg_type;
    msg->session_id = session_id;
    msg->target_job_id = target_job_id;
    msg->source_job_id = source_job_id;
}

void sk_client_msg_destroy(sk_client_msg_t* msg) {
    if (!msg) return;
    free(msg->body);
    free(msg);
}

void sk_client_msg_set_data(sk_client_msg_t* msg, const uint8_t* data, size_t len) {
    if (!msg) return;
    free(msg->body);
    msg->body = NULL;
    msg->body_len = 0;
    if (data && len > 0) {
        msg->body = (uint8_t*)malloc(len);
        if (msg->body) {
            memcpy(msg->body, data, len);
            msg->body_len = len;
        }
    }
}

const uint8_t* sk_client_msg_body(const sk_client_msg_t* msg, size_t* out_len) {
    if (!msg) return NULL;
    if (out_len) *out_len = msg->body_len;
    return msg->body;
}

bool sk_client_msg_is_proto(const sk_client_msg_t* msg) {
    return msg ? msg->is_proto : false;
}

sk_emsg_t sk_client_msg_msg_type(const sk_client_msg_t* msg) {
    return msg ? msg->msg_type : SK_EMSG_INVALID;
}

int sk_client_msg_session_id(const sk_client_msg_t* msg) {
    return msg ? msg->session_id : 0;
}

void sk_client_msg_set_session_id(sk_client_msg_t* msg, int session_id) {
    if (msg) msg->session_id = session_id;
}

const sk_steam_id_t* sk_client_msg_steam_id(const sk_client_msg_t* msg) {
    return msg ? &msg->steam_id : NULL;
}

void sk_client_msg_set_steam_id(sk_client_msg_t* msg, const sk_steam_id_t* steam_id) {
    if (!msg || !steam_id) return;
    msg->steam_id.steamid = steam_id->steamid;
}

uint64_t sk_client_msg_target_job_id(const sk_client_msg_t* msg) {
    return msg ? msg->target_job_id : 0;
}

void sk_client_msg_set_target_job_id(sk_client_msg_t* msg, uint64_t job_id) {
    if (msg) msg->target_job_id = job_id;
}

uint64_t sk_client_msg_source_job_id(const sk_client_msg_t* msg) {
    return msg ? msg->source_job_id : 0;
}

void sk_client_msg_set_source_job_id(sk_client_msg_t* msg, uint64_t job_id) {
    if (msg) msg->source_job_id = job_id;
}

uint8_t* sk_client_msg_serialize(const sk_client_msg_t* msg, size_t* out_size) {
    if (!msg || !out_size) return NULL;
    size_t total = msg->body_len;
    uint8_t* result = (uint8_t*)malloc(total);
    if (result && msg->body && total > 0) {
        memcpy(result, msg->body, total);
    }
    *out_size = total;
    return result;
}
