#ifndef STEAMKIT_UTILS_MSG_UTIL_H
#define STEAMKIT_UTILS_MSG_UTIL_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// CRC32 calculation
uint32_t sk_msg_crc32(const uint8_t* data, size_t len);

// Write a protobuf varint to buffer
size_t sk_msg_write_varint(uint8_t* buffer, uint64_t value);

// Read a protobuf varint from buffer (returns number of bytes consumed)
size_t sk_msg_read_varint(const uint8_t* buffer, size_t len, uint64_t* out_value);

// Write a zigzag-encoded varint (for signed values)
size_t sk_msg_write_varint_signed(uint8_t* buffer, int64_t value);

// Read a zigzag-encoded varint
size_t sk_msg_read_varint_signed(const uint8_t* buffer, size_t len, int64_t* out_value);

// Write a length-delimited field header
size_t sk_msg_write_field_header(uint8_t* buffer, uint32_t field_number, uint8_t wire_type);

// Skips a field in a protobuf stream
size_t sk_msg_skip_field(const uint8_t* buffer, size_t len, uint8_t wire_type);

// Finds a protobuf field by number; returns field length for wire_type != 0
// For wire_type 0 (varint), the value is stored in *out_value
// For wire_type 2 (length-delimited), the length is returned
size_t sk_msg_find_field(const uint8_t* buffer, size_t len, uint32_t field_number, uint8_t* out_wire_type, size_t* out_field_offset, uint64_t* out_value);

// Reads a little-endian uint32 from buffer
uint32_t sk_msg_read_u32_le(const uint8_t* buffer);

// Reads a little-endian uint64 from buffer
uint64_t sk_msg_read_u64_le(const uint8_t* buffer);

// Reads a uint32 from buffer at a given byte offset
static inline uint32_t sk_msg_read_u32_at(const uint8_t* buffer, size_t offset) {
    return sk_msg_read_u32_le(buffer + offset);
}

// Copies a string (null-terminated) from buffer into a caller-provided destination
size_t sk_msg_read_string(const uint8_t* buffer, size_t len, char* dest, size_t dest_len);

#ifdef __cplusplus
}
#endif

#endif // STEAMKIT_UTILS_MSG_UTIL_H
