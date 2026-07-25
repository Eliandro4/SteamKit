#include "steamkit/utils/msg_util.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>

uint32_t sk_msg_crc32(const uint8_t* data, size_t len) {
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < len; ++i) {
        crc ^= (uint32_t)data[i];
        for (int j = 0; j < 8; ++j) {
            if (crc & 1) crc = (crc >> 1) ^ 0xEDB88320;
            else crc >>= 1;
        }
    }
    return ~crc;
}

size_t sk_msg_write_varint(uint8_t* buffer, uint64_t value) {
    size_t i = 0;
    while (value > 0x7F) {
        buffer[i++] = (uint8_t)((value & 0x7F) | 0x80);
        value >>= 7;
    }
    buffer[i++] = (uint8_t)(value & 0x7F);
    return i;
}

size_t sk_msg_read_varint(const uint8_t* buffer, size_t len, uint64_t* out_value) {
    if (!buffer || !out_value || len == 0) return 0;
    *out_value = 0;
    size_t shift = 0;
    for (size_t i = 0; i < len; ++i) {
        uint8_t byte = buffer[i];
        *out_value |= ((uint64_t)(byte & 0x7F)) << shift;
        if (!(byte & 0x80)) return i + 1;
        shift += 7;
        if (shift >= 64) break;
    }
    return 0;
}

size_t sk_msg_write_varint_signed(uint8_t* buffer, int64_t value) {
    return sk_msg_write_varint(buffer, (uint64_t)((value << 1) ^ (value >> 63)));
}

size_t sk_msg_read_varint_signed(const uint8_t* buffer, size_t len, int64_t* out_value) {
    uint64_t uvalue;
    size_t consumed = sk_msg_read_varint(buffer, len, &uvalue);
    if (consumed == 0) return 0;
    *out_value = (int64_t)((uvalue >> 1) ^ -(int64_t)(uvalue & 1));
    return consumed;
}

size_t sk_msg_write_field_header(uint8_t* buffer, uint32_t field_number, uint8_t wire_type) {
    return sk_msg_write_varint(buffer, (uint64_t)((field_number << 3) | wire_type));
}

size_t sk_msg_find_field(const uint8_t* buffer, size_t len, uint32_t field_number, uint8_t* out_wire_type, size_t* out_field_offset, uint64_t* out_value) {
    if (!buffer || len == 0 || !out_wire_type || !out_field_offset) return 0;
    size_t offset = 0;
    while (offset < len) {
        uint64_t tag;
        size_t tag_len = sk_msg_read_varint(buffer + offset, len - offset, &tag);
        if (tag_len == 0) break;
        uint32_t fn = (uint32_t)(tag >> 3);
        uint8_t wt = (uint8_t)(tag & 0x7);
        size_t field_start = offset + tag_len;
        uint64_t value = 0;
        size_t field_len = 0;
        switch (wt) {
            case 0: {
                size_t vl = sk_msg_read_varint(buffer + field_start, len - field_start, &value);
                if (vl == 0) return 0;
                field_len = vl;
                break;
            }
            case 1: field_len = 8; break;
            case 2: {
                uint64_t l;
                size_t ll = sk_msg_read_varint(buffer + field_start, len - field_start, &l);
                if (ll == 0) return 0;
                field_len = ll + (size_t)l;
                break;
            }
            case 5: field_len = 4; break;
            default: return 0;
        }
        if (offset + tag_len + field_len > len) return 0;
        if (fn == field_number) {
            *out_wire_type = wt;
            *out_field_offset = field_start;
            if (out_value) *out_value = value;
            return field_len;
        }
        offset = field_start + field_len;
    }
    return 0;
}

size_t sk_msg_skip_field(const uint8_t* buffer, size_t len, uint8_t wire_type) {
    size_t i = 0;
    switch (wire_type) {
        case 0: {
            uint64_t dummy;
            i = sk_msg_read_varint(buffer, len, &dummy);
            break;
        }
        case 1: i = 8; break;
        case 2: {
            uint64_t length;
            size_t varint_len = sk_msg_read_varint(buffer, len, &length);
            if (varint_len == 0) return 0;
            i = varint_len + (size_t)length;
            break;
        }
        case 5: i = 4; break;
        default: return 0;
    }
    return i > len ? 0 : i;
}

uint32_t sk_msg_read_u32_le(const uint8_t* buffer) {
    return (uint32_t)buffer[0] |
           ((uint32_t)buffer[1] << 8) |
           ((uint32_t)buffer[2] << 16) |
           ((uint32_t)buffer[3] << 24);
}

uint64_t sk_msg_read_u64_le(const uint8_t* buffer) {
    uint64_t val = 0;
    for (int i = 0; i < 8; ++i) {
        val |= ((uint64_t)buffer[i]) << (i * 8);
    }
    return val;
}

size_t sk_msg_read_string(const uint8_t* buffer, size_t len, char* dest, size_t dest_len) {
    if (!buffer || !dest || dest_len == 0 || len == 0) return 0;
    size_t copy_len = len < dest_len - 1 ? len : dest_len - 1;
    memcpy(dest, buffer, copy_len);
    dest[copy_len] = '\0';
    return copy_len;
}
