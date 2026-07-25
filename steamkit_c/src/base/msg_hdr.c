#include "steamkit/base/msg_hdr.h"
#include <string.h>

size_t sk_msg_hdr_serialize(const sk_msg_hdr_t* hdr, uint8_t* buffer, size_t buffer_size) {
    if (!hdr || !buffer || buffer_size < 20) return 0;
    uint32_t* p = (uint32_t*)buffer;
    p[0] = (uint32_t)hdr->msg;
    p[1] = (uint32_t)(hdr->target_job_id.base.value & 0xFFFFFFFF);
    p[2] = (uint32_t)((hdr->target_job_id.base.value >> 32) & 0xFFFFFFFF);
    p[3] = (uint32_t)(hdr->source_job_id.base.value & 0xFFFFFFFF);
    p[4] = (uint32_t)((hdr->source_job_id.base.value >> 32) & 0xFFFFFFFF);
    return 20;
}

bool sk_msg_hdr_deserialize(sk_msg_hdr_t* hdr, const uint8_t* buffer, size_t buffer_size) {
    if (!hdr || !buffer || buffer_size < 20) return false;
    const uint32_t* p = (const uint32_t*)buffer;
    hdr->msg = (sk_emsg_t)p[0];
    hdr->target_job_id.base.value = ((uint64_t)p[2] << 32) | p[1];
    hdr->source_job_id.base.value = ((uint64_t)p[4] << 32) | p[3];
    return true;
}
