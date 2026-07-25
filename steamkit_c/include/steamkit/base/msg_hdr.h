#ifndef STEAMKIT_BASE_MSG_HDR_H
#define STEAMKIT_BASE_MSG_HDR_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "steamkit/types/steam_id.h"
#include "steamkit/types/job_id.h"
#include "steamkit/base/emsg.h"

#ifdef __cplusplus
extern "C" {
#endif

// Compact 4-byte header used for legacy messages
typedef struct sk_msg_hdr {
    sk_emsg_t msg;
    sk_job_id_t target_job_id;
    sk_job_id_t source_job_id;
} sk_msg_hdr_t;

// Serializes the header into a buffer
size_t sk_msg_hdr_serialize(const sk_msg_hdr_t* hdr, uint8_t* buffer, size_t buffer_size);

// Deserializes the header from a buffer
bool sk_msg_hdr_deserialize(sk_msg_hdr_t* hdr, const uint8_t* buffer, size_t buffer_size);

#ifdef __cplusplus
}
#endif

#endif // STEAMKIT_BASE_MSG_HDR_H
