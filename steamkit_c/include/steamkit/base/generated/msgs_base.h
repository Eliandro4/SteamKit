#ifndef STEAMKIT_BASE_GENERATED_MSGS_BASE_H
#define STEAMKIT_BASE_GENERATED_MSGS_BASE_H

#include <stdint.h>
#include <stdbool.h>
#include "steamkit/base/emsg.h"
#include "steamkit/types/steam_id.h"

#ifdef __cplusplus
extern "C" {
#endif

// Protobuf-backed message header
typedef struct sk_msg_hdr_proto_buf {
    uint64_t steamid;
    uint64_t jobid_source;
    uint64_t jobid_target;
    int32_t client_sessionid;
} sk_msg_hdr_proto_buf_t;

static const size_t SK_MSG_HDR_PROTO_BUF_SIZE = 32;

// Extended client message header (legacy)
typedef struct sk_extended_client_msg_hdr {
    uint64_t target_job_id;
    uint64_t source_job_id;
    int32_t session_id;
    sk_steam_id_t steam_id;
} sk_extended_client_msg_hdr_t;

static const size_t SK_EXTENDED_CLIENT_MSG_HDR_SIZE = 28;

// Base message header (legacy)
typedef struct sk_msg_hdr_base {
    sk_emsg_t msg;
    uint64_t target_job_id;
    uint64_t source_job_id;
} sk_msg_hdr_base_t;

static const size_t SK_MSG_HDR_BASE_SIZE = 20;

#ifdef __cplusplus
}
#endif

#endif // STEAMKIT_BASE_GENERATED_MSGS_BASE_H
