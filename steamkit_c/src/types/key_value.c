#include <stddef.h>
#include <stdlib.h>
#include <string.h>

static void* memdup(const void* src, size_t len) {
    void* dst = malloc(len);
    if (dst && src && len > 0) {
        memcpy(dst, src, len);
    }
    return dst;
}

static char* strdup_local(const char* s) {
    if (!s) return NULL;
    size_t len = strlen(s) + 1;
    char* dst = malloc(len);
    if (dst) {
        memcpy(dst, s, len);
    }
    return dst;
}

#include "steamkit/types/key_value.h"
#include <stdarg.h>

typedef struct sk_kv_value {
    sk_kv_type_t type;
    union {
        char* str;
        int32_t i32;
        uint32_t u32;
        int64_t i64;
        uint64_t u64;
        float f32;
        double f64;
        bool b;
        struct {
            uint8_t* data;
            size_t len;
        } binary;
    } as;
} sk_kv_value_t;

typedef struct sk_key_value_child {
    sk_key_value_t* node;
    struct sk_key_value_child* next;
} sk_key_value_child_t;

struct sk_key_value {
    char* name;
    sk_kv_value_t value;
    sk_key_value_child_t* children;
    sk_key_value_child_t* children_tail;
};

sk_key_value_t* sk_key_value_create(const char* name) {
    sk_key_value_t* kv = (sk_key_value_t*)calloc(1, sizeof(sk_key_value_t));
    if (kv && name) {
        kv->name = strdup_local(name);
    }
    return kv;
}

void sk_key_value_destroy(sk_key_value_t* kv) {
    if (!kv) return;
    free(kv->name);
    if (kv->value.type == SK_KV_TYPE_STRING || kv->value.type == SK_KV_TYPE_WSTRING) {
        free(kv->value.as.str);
    }
    if (kv->value.type == SK_KV_TYPE_BINARY_BLOB) {
        free(kv->value.as.binary.data);
    }
    // Destroy children
    sk_key_value_child_t* child = kv->children;
    while (child) {
        sk_key_value_child_t* next = child->next;
        sk_key_value_destroy(child->node);
        free(child);
        child = next;
    }
    free(kv);
}

const char* sk_key_value_name(const sk_key_value_t* kv) {
    return kv ? kv->name : NULL;
}

void sk_key_value_set_name(sk_key_value_t* kv, const char* name) {
    if (!kv) return;
    free(kv->name);
    kv->name = name ? strdup_local(name) : NULL;
}

sk_kv_type_t sk_key_value_type(const sk_key_value_t* kv) {
    return kv ? kv->value.type : SK_KV_TYPE_NONE;
}

void sk_key_value_set_type(sk_key_value_t* kv, sk_kv_type_t type) {
    if (!kv) return;
    // Free old value
    if (kv->value.type == SK_KV_TYPE_STRING || kv->value.type == SK_KV_TYPE_WSTRING) {
        free(kv->value.as.str);
    }
    if (kv->value.type == SK_KV_TYPE_BINARY_BLOB) {
        free(kv->value.as.binary.data);
    }
    memset(&kv->value, 0, sizeof(kv->value));
    kv->value.type = type;
}

// Value accessors
void sk_key_value_set_string(sk_key_value_t* kv, const char* value) {
    if (!kv) return;
    free(kv->value.as.str);
    kv->value.as.str = value ? strdup_local(value) : NULL;
    kv->value.type = SK_KV_TYPE_STRING;
}

const char* sk_key_value_string(const sk_key_value_t* kv) {
    return kv ? kv->value.as.str : NULL;
}

void sk_key_value_set_int32(sk_key_value_t* kv, int32_t value) {
    if (kv) { kv->value.as.i32 = value; kv->value.type = SK_KV_TYPE_INT32; }
}

int32_t sk_key_value_int32(const sk_key_value_t* kv) {
    return kv ? kv->value.as.i32 : 0;
}

void sk_key_value_set_uint32(sk_key_value_t* kv, uint32_t value) {
    if (kv) { kv->value.as.u32 = value; kv->value.type = SK_KV_TYPE_UINT32; }
}

uint32_t sk_key_value_uint32(const sk_key_value_t* kv) {
    return kv ? kv->value.as.u32 : 0;
}

void sk_key_value_set_int64(sk_key_value_t* kv, int64_t value) {
    if (kv) { kv->value.as.i64 = value; kv->value.type = SK_KV_TYPE_INT64; }
}

int64_t sk_key_value_int64(const sk_key_value_t* kv) {
    return kv ? kv->value.as.i64 : 0;
}

void sk_key_value_set_uint64(sk_key_value_t* kv, uint64_t value) {
    if (kv) { kv->value.as.u64 = value; kv->value.type = SK_KV_TYPE_UINT64; }
}

uint64_t sk_key_value_uint64(const sk_key_value_t* kv) {
    return kv ? kv->value.as.u64 : 0;
}

void sk_key_value_set_float(sk_key_value_t* kv, float value) {
    if (kv) { kv->value.as.f32 = value; kv->value.type = SK_KV_TYPE_FLOAT32; }
}

float sk_key_value_float(const sk_key_value_t* kv) {
    return kv ? kv->value.as.f32 : 0.0f;
}

void sk_key_value_set_double(sk_key_value_t* kv, double value) {
    if (kv) { kv->value.as.f64 = value; kv->value.type = SK_KV_TYPE_FLOAT64; }
}

double sk_key_value_double(const sk_key_value_t* kv) {
    return kv ? kv->value.as.f64 : 0.0;
}

void sk_key_value_set_bool(sk_key_value_t* kv, bool value) {
    if (kv) { kv->value.as.b = value; kv->value.type = value ? SK_KV_TYPE_BOOL_TRUE : SK_KV_TYPE_BOOL_FALSE; }
}

bool sk_key_value_bool(const sk_key_value_t* kv) {
    return kv ? kv->value.as.b : false;
}

void sk_key_value_set_binary(sk_key_value_t* kv, const uint8_t* data, size_t len) {
    if (!kv) return;
    free(kv->value.as.binary.data);
    kv->value.as.binary.data = data ? (uint8_t*)memdup(data, len) : NULL;
    kv->value.as.binary.len = len;
    kv->value.type = SK_KV_TYPE_BINARY_BLOB;
}

const uint8_t* sk_key_value_binary(const sk_key_value_t* kv, size_t* out_len) {
    if (!kv) return NULL;
    if (out_len) *out_len = kv->value.as.binary.len;
    return kv->value.as.binary.data;
}

// Children
size_t sk_key_value_child_count(const sk_key_value_t* kv) {
    if (!kv) return 0;
    size_t count = 0;
    sk_key_value_child_t* child = kv->children;
    while (child) {
        count++;
        child = child->next;
    }
    return count;
}

sk_key_value_t* sk_key_value_child(const sk_key_value_t* kv, size_t index) {
    if (!kv) return NULL;
    sk_key_value_child_t* child = kv->children;
    while (child && index > 0) {
        child = child->next;
        index--;
    }
    return child ? child->node : NULL;
}

sk_key_value_t* sk_key_value_child_named(const sk_key_value_t* kv, const char* name) {
    if (!kv || !name) return NULL;
    sk_key_value_child_t* child = kv->children;
    while (child) {
        if (child->node && child->node->name && strcmp(child->node->name, name) == 0) {
            return child->node;
        }
        child = child->next;
    }
    return NULL;
}

void sk_key_value_add_child(sk_key_value_t* kv, sk_key_value_t* child) {
    if (!kv || !child) return;
    sk_key_value_child_t* entry = (sk_key_value_child_t*)malloc(sizeof(sk_key_value_child_t));
    if (!entry) return;
    entry->node = child;
    entry->next = NULL;
    if (kv->children_tail) {
        kv->children_tail->next = entry;
    } else {
        kv->children = entry;
    }
    kv->children_tail = entry;
}

size_t sk_key_value_serialize(const sk_key_value_t* kv, uint8_t* buffer, size_t buffer_size) {
    if (!kv || !buffer || buffer_size == 0) return 0;
    size_t written = 0;

    uint8_t type = (uint8_t)kv->value.type;
    if (written + 1 > buffer_size) return 0;
    buffer[written++] = type;

    size_t name_len = kv->name ? strlen(kv->name) : 0;
    if (name_len > 255) name_len = 255;
    if (written + 1 > buffer_size) return 0;
    buffer[written++] = (uint8_t)name_len;
    if (name_len > 0) {
        if (written + name_len > buffer_size) return 0;
        memcpy(buffer + written, kv->name, name_len);
        written += name_len;
    }

    switch (kv->value.type) {
        case SK_KV_TYPE_STRING: {
            size_t slen = kv->value.as.str ? strlen(kv->value.as.str) : 0;
            if (written + 4 + slen > buffer_size) return 0;
            uint32_t len = (uint32_t)slen;
            buffer[written++] = len & 0xFF;
            buffer[written++] = (len >> 8) & 0xFF;
            buffer[written++] = (len >> 16) & 0xFF;
            buffer[written++] = (len >> 24) & 0xFF;
            if (slen > 0) {
                memcpy(buffer + written, kv->value.as.str, slen);
                written += slen;
            }
            break;
        }
        case SK_KV_TYPE_INT32:
        case SK_KV_TYPE_UINT32:
        case SK_KV_TYPE_COLOR: {
            if (written + 4 > buffer_size) return 0;
            uint32_t val = kv->value.type == SK_KV_TYPE_INT32 ? (uint32_t)kv->value.as.i32 :
                           kv->value.type == SK_KV_TYPE_UINT32 ? kv->value.as.u32 :
                           kv->value.as.u32;
            buffer[written++] = val & 0xFF;
            buffer[written++] = (val >> 8) & 0xFF;
            buffer[written++] = (val >> 16) & 0xFF;
            buffer[written++] = (val >> 24) & 0xFF;
            break;
        }
        case SK_KV_TYPE_BOOL_TRUE: {
            if (written + 4 > buffer_size) return 0;
            buffer[written++] = 1;
            buffer[written++] = 0;
            buffer[written++] = 0;
            buffer[written++] = 0;
            break;
        }
        case SK_KV_TYPE_BOOL_FALSE: {
            if (written + 4 > buffer_size) return 0;
            buffer[written++] = 0;
            buffer[written++] = 0;
            buffer[written++] = 0;
            buffer[written++] = 0;
            break;
        }
        case SK_KV_TYPE_NULL:
        case SK_KV_TYPE_NONE: {
            break;
        }
        case SK_KV_TYPE_INT64:
        case SK_KV_TYPE_UINT64: {
            if (written + 8 > buffer_size) return 0;
            uint64_t val = kv->value.type == SK_KV_TYPE_INT64 ? (uint64_t)kv->value.as.i64 : kv->value.as.u64;
            for (int i = 0; i < 8; ++i) {
                buffer[written++] = (val >> (i * 8)) & 0xFF;
            }
            break;
        }
        case SK_KV_TYPE_FLOAT32: {
            if (written + 4 > buffer_size) return 0;
            union { float f; uint32_t u; } conv;
            conv.f = kv->value.as.f32;
            uint32_t val = conv.u;
            buffer[written++] = val & 0xFF;
            buffer[written++] = (val >> 8) & 0xFF;
            buffer[written++] = (val >> 16) & 0xFF;
            buffer[written++] = (val >> 24) & 0xFF;
            break;
        }
        case SK_KV_TYPE_FLOAT64: {
            if (written + 8 > buffer_size) return 0;
            union { double d; uint64_t u; } conv;
            conv.d = kv->value.as.f64;
            uint64_t val = conv.u;
            for (int i = 0; i < 8; ++i) {
                buffer[written++] = (val >> (i * 8)) & 0xFF;
            }
            break;
        }
        case SK_KV_TYPE_BINARY_BLOB: {
            if (written + 4 + kv->value.as.binary.len > buffer_size) return 0;
            uint32_t len = (uint32_t)kv->value.as.binary.len;
            buffer[written++] = len & 0xFF;
            buffer[written++] = (len >> 8) & 0xFF;
            buffer[written++] = (len >> 16) & 0xFF;
            buffer[written++] = (len >> 24) & 0xFF;
            if (len > 0 && kv->value.as.binary.data) {
                memcpy(buffer + written, kv->value.as.binary.data, len);
                written += len;
            }
            break;
        }
        default: break;
    }

    sk_key_value_child_t* child = kv->children;
    while (child) {
        size_t child_written = sk_key_value_serialize(child->node, buffer + written, buffer_size - written);
        if (child_written == 0) return 0;
        written += child_written;
        child = child->next;
    }

    if (written + 1 > buffer_size) return 0;
    buffer[written++] = 0;

    return written;
}

size_t sk_key_value_deserialize(sk_key_value_t* kv, const uint8_t* buffer, size_t buffer_size) {
    if (!kv || !buffer || buffer_size == 0) return 0;
    size_t offset = 0;

    if (offset + 1 > buffer_size) return 0;
    uint8_t type = buffer[offset++];

    if (offset + 1 > buffer_size) return 0;
    uint8_t name_len = buffer[offset++];
    if (name_len > 0) {
        if (offset + name_len > buffer_size) return 0;
        char name[256];
        memcpy(name, buffer + offset, name_len);
        name[name_len] = 0;
        sk_key_value_set_name(kv, name);
        offset += name_len;
    }

    if (type != 0) {
        switch (type) {
            case SK_KV_TYPE_STRING: {
                if (offset + 4 > buffer_size) return 0;
                uint32_t len = (uint32_t)buffer[offset] |
                               ((uint32_t)buffer[offset + 1] << 8) |
                               ((uint32_t)buffer[offset + 2] << 16) |
                               ((uint32_t)buffer[offset + 3] << 24);
                offset += 4;
                if (offset + len > buffer_size) return 0;
                char* str = (char*)malloc(len + 1);
                if (str) {
                    memcpy(str, buffer + offset, len);
                    str[len] = 0;
                    sk_key_value_set_string(kv, str);
                    free(str);
                }
                offset += len;
                break;
            }
            case SK_KV_TYPE_INT32: {
                if (offset + 4 > buffer_size) return 0;
                int32_t val = (int32_t)((uint32_t)buffer[offset] |
                                        ((uint32_t)buffer[offset + 1] << 8) |
                                        ((uint32_t)buffer[offset + 2] << 16) |
                                        ((uint32_t)buffer[offset + 3] << 24));
                sk_key_value_set_int32(kv, val);
                offset += 4;
                break;
            }
            case SK_KV_TYPE_UINT32:
            case SK_KV_TYPE_COLOR: {
                if (offset + 4 > buffer_size) return 0;
                uint32_t val = (uint32_t)buffer[offset] |
                               ((uint32_t)buffer[offset + 1] << 8) |
                               ((uint32_t)buffer[offset + 2] << 16) |
                               ((uint32_t)buffer[offset + 3] << 24);
                sk_key_value_set_uint32(kv, val);
                offset += 4;
                break;
            }
            case SK_KV_TYPE_BOOL_TRUE: {
                sk_key_value_set_bool(kv, true);
                offset += 4;
                break;
            }
            case SK_KV_TYPE_BOOL_FALSE: {
                sk_key_value_set_bool(kv, false);
                offset += 4;
                break;
            }
            case SK_KV_TYPE_INT64: {
                if (offset + 8 > buffer_size) return 0;
                uint64_t val = 0;
                for (int i = 0; i < 8; ++i) {
                    val |= ((uint64_t)buffer[offset + i]) << (i * 8);
                }
                sk_key_value_set_int64(kv, (int64_t)val);
                offset += 8;
                break;
            }
            case SK_KV_TYPE_UINT64: {
                if (offset + 8 > buffer_size) return 0;
                uint64_t val = 0;
                for (int i = 0; i < 8; ++i) {
                    val |= ((uint64_t)buffer[offset + i]) << (i * 8);
                }
                sk_key_value_set_uint64(kv, val);
                offset += 8;
                break;
            }
            case SK_KV_TYPE_FLOAT32: {
                if (offset + 4 > buffer_size) return 0;
                uint32_t val = (uint32_t)buffer[offset] |
                               ((uint32_t)buffer[offset + 1] << 8) |
                               ((uint32_t)buffer[offset + 2] << 16) |
                               ((uint32_t)buffer[offset + 3] << 24);
                union { uint32_t u; float f; } conv;
                conv.u = val;
                sk_key_value_set_float(kv, conv.f);
                offset += 4;
                break;
            }
            case SK_KV_TYPE_FLOAT64: {
                if (offset + 8 > buffer_size) return 0;
                uint64_t val = 0;
                for (int i = 0; i < 8; ++i) {
                    val |= ((uint64_t)buffer[offset + i]) << (i * 8);
                }
                union { uint64_t u; double d; } conv;
                conv.u = val;
                sk_key_value_set_double(kv, conv.d);
                offset += 8;
                break;
            }
            case SK_KV_TYPE_BINARY_BLOB: {
                if (offset + 4 > buffer_size) return 0;
                uint32_t len = (uint32_t)buffer[offset] |
                               ((uint32_t)buffer[offset + 1] << 8) |
                               ((uint32_t)buffer[offset + 2] << 16) |
                               ((uint32_t)buffer[offset + 3] << 24);
                offset += 4;
                if (offset + len > buffer_size) return 0;
                sk_key_value_set_binary(kv, buffer + offset, len);
                offset += len;
                break;
            }
            default: break;
        }
    }

    while (offset + 1 <= buffer_size && buffer[offset] != 0) {
        sk_key_value_t* child = sk_key_value_create("child");
        if (!child) return 0;
        size_t child_read = sk_key_value_deserialize(child, buffer + offset, buffer_size - offset);
        if (child_read == 0) {
            sk_key_value_destroy(child);
            break;
        }
        offset += child_read;
        sk_key_value_add_child(kv, child);
    }

    if (offset + 1 <= buffer_size && buffer[offset] == 0) {
        offset++;
    }

    return offset;
}

bool sk_key_value_load_from_buffer(sk_key_value_t* kv, const uint8_t* buffer, size_t buffer_size, bool as_binary) {
    (void)as_binary;
    if (!kv || !buffer || buffer_size == 0) return false;
    sk_key_value_deserialize(kv, buffer, buffer_size);
    return true;
}

size_t sk_key_value_save_to_buffer(const sk_key_value_t* kv, uint8_t* buffer, size_t buffer_size, bool as_binary) {
    (void)as_binary;
    return sk_key_value_serialize(kv, buffer, buffer_size);
}
