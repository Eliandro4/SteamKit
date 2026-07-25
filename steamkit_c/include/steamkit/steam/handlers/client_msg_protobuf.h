#ifndef STEAMKIT_STEAM_HANDLERS_CLIENT_MSG_PROTOBUF_H
#define STEAMKIT_STEAM_HANDLERS_CLIENT_MSG_PROTOBUF_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "steamkit/base/emsg.h"
#include "steamkit/base/packet_base.h"
#include "steamkit/base/client_msg.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CMsgProtoBufHeader CMsgProtoBufHeader;

typedef struct sk_client_msg_protobuf sk_client_msg_protobuf_t;

// Creates a protobuf-backed client message
sk_client_msg_protobuf_t* sk_client_msg_protobuf_create(sk_emsg_t msg_type);

// Creates a protobuf-backed client message from a received packet
sk_client_msg_protobuf_t* sk_client_msg_protobuf_create_from_packet(const sk_packet_msg_t* packet_msg);

// Destroy a protobuf client message
void sk_client_msg_protobuf_destroy(sk_client_msg_protobuf_t* msg);

// Get the message type
sk_emsg_t sk_client_msg_protobuf_msg_type(const sk_client_msg_protobuf_t* msg);

// Sets the protobuf message body
void sk_client_msg_protobuf_set_body(sk_client_msg_protobuf_t* msg, const uint8_t* body, size_t body_len);

// Gets the protobuf message body
const uint8_t* sk_client_msg_protobuf_get_body(const sk_client_msg_protobuf_t* msg, size_t* out_len);

// Access the underlying header to set routing info, job ids, etc.
CMsgProtoBufHeader* sk_client_msg_protobuf_header(sk_client_msg_protobuf_t* msg);

// Converts to a packet msg for sending
sk_packet_msg_t* sk_packet_msg_create_from_client_msg_protobuf(const sk_client_msg_protobuf_t* msg);

#ifdef __cplusplus
}
#endif

#endif // STEAMKIT_STEAM_HANDLERS_CLIENT_MSG_PROTOBUF_H
