#include "steamkit/base/packet_base.h"
#include "steamkit/base/client_msg.h"
#include <stdlib.h>
#include <string.h>

sk_packet_msg_t* sk_packet_msg_create(uint32_t msg_type, bool is_proto) {
    sk_packet_msg_t* msg = (sk_packet_msg_t*)calloc(1, sizeof(sk_packet_msg_t));
    if (msg) {
        msg->msg_type = msg_type;
        msg->is_proto = is_proto;
        msg->protocol = SK_PROTOCOL_TYPE_TCP;
    }
    return msg;
}

void sk_packet_msg_destroy(sk_packet_msg_t* msg) {
    if (!msg) return;
    free(msg->data);
    free(msg);
}

void sk_packet_msg_set_data(sk_packet_msg_t* msg, const uint8_t* data, size_t len) {
    if (!msg) return;
    free(msg->data);
    msg->data = NULL;
    msg->data_len = 0;
    if (data && len > 0) {
        msg->data = (uint8_t*)malloc(len);
        if (msg->data) {
            memcpy(msg->data, data, len);
            msg->data_len = len;
        }
    }
}

void sk_packet_msg_set_job_ids(sk_packet_msg_t* msg, uint64_t target, uint64_t source) {
    if (!msg) return;
    msg->target_job_id = target;
    msg->source_job_id = source;
}

void sk_packet_msg_set_msg_type(sk_packet_msg_t* msg, uint32_t msg_type) {
    if (!msg) return;
    msg->msg_type = msg_type;
}

sk_packet_msg_t* sk_packet_msg_create_from_client_msg(const sk_client_msg_t* client_msg) {
    if (!client_msg) return NULL;
    sk_packet_msg_t* pkt = sk_packet_msg_create((uint32_t)client_msg->msg_type, client_msg->is_proto);
    if (!pkt) return NULL;
    sk_packet_msg_set_job_ids(pkt, client_msg->target_job_id, client_msg->source_job_id);
    if (client_msg->is_proto) {
        if (client_msg->body_len > 0) {
            sk_packet_msg_set_data(pkt, client_msg->body, client_msg->body_len);
        }
    } else {
        sk_msg_hdr_t hdr;
        hdr.msg = client_msg->msg_type;
        hdr.target_job_id.base.value = client_msg->target_job_id;
        hdr.source_job_id.base.value = client_msg->source_job_id;
        uint8_t hdr_buf[20];
        size_t hdr_len = sk_msg_hdr_serialize(&hdr, hdr_buf, sizeof(hdr_buf));
        size_t total_len = hdr_len + client_msg->body_len;
        uint8_t* combined = (uint8_t*)malloc(total_len);
        if (combined) {
            memcpy(combined, hdr_buf, hdr_len);
            if (client_msg->body_len > 0 && client_msg->body) {
                memcpy(combined + hdr_len, client_msg->body, client_msg->body_len);
            }
            sk_packet_msg_set_data(pkt, combined, total_len);
            free(combined);
        }
    }
    return pkt;
}

sk_packet_msg_t* sk_packet_msg_create_from_buffer(const uint8_t* buffer, size_t len) {
    if (!buffer || len < 20) return NULL;
    uint32_t emsg;
    memcpy(&emsg, buffer, 4);
    sk_packet_msg_t* pkt = sk_packet_msg_create(emsg, false);
    if (!pkt) return NULL;
    sk_msg_hdr_t hdr;
    if (sk_msg_hdr_deserialize(&hdr, buffer, len)) {
        sk_packet_msg_set_job_ids(pkt, hdr.target_job_id.base.value, hdr.source_job_id.base.value);
        if (len > 20) {
            sk_packet_msg_set_data(pkt, buffer + 20, len - 20);
        }
    } else {
        sk_packet_msg_set_data(pkt, buffer, len);
    }
    return pkt;
}

bool sk_packet_msg_is_proto(const sk_packet_msg_t* msg) {
    return msg ? msg->is_proto : false;
}

uint32_t sk_packet_msg_msg_type(const sk_packet_msg_t* msg) {
    return msg ? msg->msg_type : 0;
}

uint64_t sk_packet_msg_target_job_id(const sk_packet_msg_t* msg) {
    return msg ? msg->target_job_id : 0;
}

uint64_t sk_packet_msg_source_job_id(const sk_packet_msg_t* msg) {
    return msg ? msg->source_job_id : 0;
}

const uint8_t* sk_packet_msg_data(const sk_packet_msg_t* msg, size_t* out_size) {
    if (!msg) return NULL;
    if (out_size) *out_size = msg->data_len;
    return msg->data;
}

size_t sk_packet_msg_body_offset(const sk_packet_msg_t* msg) {
    if (!msg) return 0;
    if (msg->is_proto) return 32;
    return 0;
}
