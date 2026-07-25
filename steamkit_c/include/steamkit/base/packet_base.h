#ifndef STEAMKIT_BASE_PACKET_BASE_H
#define STEAMKIT_BASE_PACKET_BASE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
struct sk_client_msg;
typedef struct sk_client_msg sk_client_msg_t;

typedef enum sk_protocol_type {
    SK_PROTOCOL_TYPE_TCP = 0,
    SK_PROTOCOL_TYPE_UDP = 1,
    SK_PROTOCOL_TYPE_WEBSOCKET = 2
} sk_protocol_type_t;

// Packet message interface - mirrors C# IPacketMsg
typedef struct sk_packet_msg sk_packet_msg_t;

struct sk_packet_msg {
    bool is_proto;
    sk_protocol_type_t protocol;
    void* vtable;
    void* impl;
    uint32_t msg_type;
    uint8_t* data;
    size_t data_len;
    uint64_t target_job_id;
    uint64_t source_job_id;
};

// Packet message functions
sk_packet_msg_t* sk_packet_msg_create(uint32_t msg_type, bool is_proto);
sk_packet_msg_t* sk_packet_msg_create_from_client_msg(const sk_client_msg_t* msg);
sk_packet_msg_t* sk_packet_msg_create_from_buffer(const uint8_t* buffer, size_t len);
void sk_packet_msg_destroy(sk_packet_msg_t* msg);
void sk_packet_msg_set_data(sk_packet_msg_t* msg, const uint8_t* data, size_t len);
void sk_packet_msg_set_job_ids(sk_packet_msg_t* msg, uint64_t target, uint64_t source);
void sk_packet_msg_set_msg_type(sk_packet_msg_t* msg, uint32_t msg_type);

bool sk_packet_msg_is_proto(const sk_packet_msg_t* msg);
uint32_t sk_packet_msg_msg_type(const sk_packet_msg_t* msg);
uint64_t sk_packet_msg_target_job_id(const sk_packet_msg_t* msg);
uint64_t sk_packet_msg_source_job_id(const sk_packet_msg_t* msg);
const uint8_t* sk_packet_msg_data(const sk_packet_msg_t* msg, size_t* out_size);
size_t sk_packet_msg_body_offset(const sk_packet_msg_t* msg);

#ifdef __cplusplus
}
#endif

#endif // STEAMKIT_BASE_PACKET_BASE_H
