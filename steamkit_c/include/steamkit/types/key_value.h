#ifndef STEAMKIT_TYPES_KEY_VALUE_H
#define STEAMKIT_TYPES_KEY_VALUE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Value types for KeyValue
typedef enum sk_kv_type {
    SK_KV_TYPE_NONE = 0,
    SK_KV_TYPE_STRING = 1,
    SK_KV_TYPE_INT32 = 2,
    SK_KV_TYPE_UINT32 = 3,
    SK_KV_TYPE_FLOAT32 = 4,
    SK_KV_TYPE_PTR = 5,
    SK_KV_TYPE_WSTRING = 6,
    SK_KV_TYPE_COLOR = 7,
    SK_KV_TYPE_INT64 = 8,
    SK_KV_TYPE_UINT64 = 9,
    SK_KV_TYPE_NULL = 10,
    SK_KV_TYPE_BINARY_BLOB = 11,
    SK_KV_TYPE_BOOL_TRUE = 12,
    SK_KV_TYPE_BOOL_FALSE = 13,
    SK_KV_TYPE_INT32_2 = 14,
    SK_KV_TYPE_FLOAT64 = 15,
} sk_kv_type_t;

// KeyValue node
typedef struct sk_key_value sk_key_value_t;

sk_key_value_t* sk_key_value_create(const char* name);
void sk_key_value_destroy(sk_key_value_t* kv);

const char* sk_key_value_name(const sk_key_value_t* kv);
void sk_key_value_set_name(sk_key_value_t* kv, const char* name);

sk_kv_type_t sk_key_value_type(const sk_key_value_t* kv);
void sk_key_value_set_type(sk_key_value_t* kv, sk_kv_type_t type);

// Value accessors
void sk_key_value_set_string(sk_key_value_t* kv, const char* value);
const char* sk_key_value_string(const sk_key_value_t* kv);

void sk_key_value_set_int32(sk_key_value_t* kv, int32_t value);
int32_t sk_key_value_int32(const sk_key_value_t* kv);

void sk_key_value_set_uint32(sk_key_value_t* kv, uint32_t value);
uint32_t sk_key_value_uint32(const sk_key_value_t* kv);

void sk_key_value_set_int64(sk_key_value_t* kv, int64_t value);
int64_t sk_key_value_int64(const sk_key_value_t* kv);

void sk_key_value_set_uint64(sk_key_value_t* kv, uint64_t value);
uint64_t sk_key_value_uint64(const sk_key_value_t* kv);

void sk_key_value_set_float(sk_key_value_t* kv, float value);
float sk_key_value_float(const sk_key_value_t* kv);

void sk_key_value_set_double(sk_key_value_t* kv, double value);
double sk_key_value_double(const sk_key_value_t* kv);

void sk_key_value_set_bool(sk_key_value_t* kv, bool value);
bool sk_key_value_bool(const sk_key_value_t* kv);

void sk_key_value_set_binary(sk_key_value_t* kv, const uint8_t* data, size_t len);
const uint8_t* sk_key_value_binary(const sk_key_value_t* kv, size_t* out_len);

// Children
size_t sk_key_value_child_count(const sk_key_value_t* kv);
sk_key_value_t* sk_key_value_child(const sk_key_value_t* kv, size_t index);
sk_key_value_t* sk_key_value_child_named(const sk_key_value_t* kv, const char* name);
void sk_key_value_add_child(sk_key_value_t* kv, sk_key_value_t* child);

size_t sk_key_value_serialize(const sk_key_value_t* kv, uint8_t* buffer, size_t buffer_size);
size_t sk_key_value_deserialize(sk_key_value_t* kv, const uint8_t* buffer, size_t buffer_size);

bool sk_key_value_load_from_buffer(sk_key_value_t* kv, const uint8_t* buffer, size_t buffer_size, bool as_binary);
size_t sk_key_value_save_to_buffer(const sk_key_value_t* kv, uint8_t* buffer, size_t buffer_size, bool as_binary);

// VDF format parse/write
char* sk_key_value_write_vdf(const sk_key_value_t* kv, size_t* out_len);
sk_key_value_t* sk_key_value_parse_vdf(const char* text, size_t text_len);
sk_key_value_t* sk_key_value_load_vdf_file(const char* path);
bool sk_key_value_save_vdf_file(const sk_key_value_t* kv, const char* path);

#ifdef __cplusplus
}
#endif

#endif // STEAMKIT_TYPES_KEY_VALUE_H
