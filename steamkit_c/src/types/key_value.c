#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
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

static size_t sk_vdf_grow_buffer(char** buf, size_t* cap, size_t* len, const char* str, size_t slen) {
    while (*len + slen >= *cap) {
        size_t new_cap = *cap == 0 ? 1024 : *cap * 2;
        char* new_buf = (char*)realloc(*buf, new_cap);
        if (!new_buf) return 0;
        *buf = new_buf;
        *cap = new_cap;
    }
    memcpy(*buf + *len, str, slen);
    *len += slen;
    return slen;
}

static void sk_vdf_write_value(char** buf, size_t* cap, size_t* len, const sk_key_value_t* kv);

static void sk_vdf_write_key(char** buf, size_t* cap, size_t* len, const char* key) {
    sk_vdf_grow_buffer(buf, cap, len, "\"", 1);
    sk_vdf_grow_buffer(buf, cap, len, key, strlen(key));
    sk_vdf_grow_buffer(buf, cap, len, "\"", 1);
}

static void sk_vdf_write_string_value(char** buf, size_t* cap, size_t* len, const char* value) {
    sk_vdf_grow_buffer(buf, cap, len, "\"", 1);
    if (value) {
        for (const char* p = value; *p; ++p) {
            if (*p == '\\' || *p == '"' || *p == '\n' || *p == '\r' || *p == '\t') {
                sk_vdf_grow_buffer(buf, cap, len, "\\", 1);
                char esc[2] = { 0, 0 };
                switch (*p) {
                    case '\\': esc[0] = '\\'; break;
                    case '"': esc[0] = '"'; break;
                    case '\n': esc[0] = 'n'; break;
                    case '\r': esc[0] = 'r'; break;
                    case '\t': esc[0] = 't'; break;
                }
                sk_vdf_grow_buffer(buf, cap, len, esc, 1);
            } else {
                char cbuf[2] = { *p, 0 };
                sk_vdf_grow_buffer(buf, cap, len, cbuf, 1);
            }
        }
    }
    sk_vdf_grow_buffer(buf, cap, len, "\"", 1);
}

static void sk_vdf_write_int_value(char** buf, size_t* cap, size_t* len, int64_t value) {
    char tmp[32];
    int n = snprintf(tmp, sizeof(tmp), "%lld", (long long)value);
    sk_vdf_grow_buffer(buf, cap, len, tmp, (size_t)n);
}

static void sk_vdf_write_uint_value(char** buf, size_t* cap, size_t* len, uint64_t value) {
    char tmp[32];
    int n = snprintf(tmp, sizeof(tmp), "%llu", (unsigned long long)value);
    sk_vdf_grow_buffer(buf, cap, len, tmp, (size_t)n);
}

static void sk_vdf_write_float_value(char** buf, size_t* cap, size_t* len, double value) {
    char tmp[64];
    int n = snprintf(tmp, sizeof(tmp), "%g", value);
    sk_vdf_grow_buffer(buf, cap, len, tmp, (size_t)n);
}

static void sk_vdf_write_bool_value(char** buf, size_t* cap, size_t* len, bool value) {
    sk_vdf_grow_buffer(buf, cap, len, value ? "1" : "0", 1);
}

static void sk_vdf_write_binary_value(char** buf, size_t* cap, size_t* len, const uint8_t* data, size_t dlen) {
    for (size_t i = 0; i < dlen; ++i) {
        char hex[3];
        snprintf(hex, sizeof(hex), "%02x", data[i]);
        sk_vdf_grow_buffer(buf, cap, len, hex, 2);
    }
}

static void sk_vdf_write_value(char** buf, size_t* cap, size_t* len, const sk_key_value_t* kv) {
    switch (kv->value.type) {
        case SK_KV_TYPE_STRING:
        case SK_KV_TYPE_WSTRING:
            sk_vdf_write_string_value(buf, cap, len, kv->value.as.str);
            break;
        case SK_KV_TYPE_INT32:
        case SK_KV_TYPE_INT64:
            sk_vdf_write_int_value(buf, cap, len, kv->value.as.i64);
            break;
        case SK_KV_TYPE_UINT32:
        case SK_KV_TYPE_UINT64:
            sk_vdf_write_uint_value(buf, cap, len, kv->value.as.u64);
            break;
        case SK_KV_TYPE_FLOAT32:
        case SK_KV_TYPE_FLOAT64:
            sk_vdf_write_float_value(buf, cap, len, kv->value.as.f64);
            break;
        case SK_KV_TYPE_BOOL_TRUE:
        case SK_KV_TYPE_BOOL_FALSE:
            sk_vdf_write_bool_value(buf, cap, len, kv->value.as.b);
            break;
        case SK_KV_TYPE_BINARY_BLOB:
            sk_vdf_write_binary_value(buf, cap, len, kv->value.as.binary.data, kv->value.as.binary.len);
            break;
        case SK_KV_TYPE_COLOR: {
            char tmp[16];
            snprintf(tmp, sizeof(tmp), "0x%08X", kv->value.as.u32);
            sk_vdf_grow_buffer(buf, cap, len, tmp, strlen(tmp));
            break;
        }
        case SK_KV_TYPE_NULL:
        case SK_KV_TYPE_NONE:
        default:
            break;
    }
}

static void sk_vdf_write_node(char** buf, size_t* cap, size_t* len, const sk_key_value_t* kv, int indent) {
    for (int i = 0; i < indent; ++i) sk_vdf_grow_buffer(buf, cap, len, "\t", 1);
    sk_vdf_write_key(buf, cap, len, kv->name);

    if (kv->value.type != SK_KV_TYPE_NONE && kv->value.type != SK_KV_TYPE_NULL) {
        sk_vdf_grow_buffer(buf, cap, len, "\t\t", 2);
        sk_vdf_write_value(buf, cap, len, kv);
        sk_vdf_grow_buffer(buf, cap, len, "\n", 1);
    } else if (kv->children) {
        sk_vdf_grow_buffer(buf, cap, len, "\n", 1);
        sk_key_value_child_t* child = kv->children;
        while (child) {
            sk_vdf_write_node(buf, cap, len, child->node, indent + 1);
            child = child->next;
        }
        for (int i = 0; i < indent; ++i) sk_vdf_grow_buffer(buf, cap, len, "\t", 1);
        sk_vdf_grow_buffer(buf, cap, len, "}\n", 2);
    } else {
        sk_vdf_grow_buffer(buf, cap, len, "\n", 1);
    }
}

char* sk_key_value_write_vdf(const sk_key_value_t* kv, size_t* out_len) {
    if (!kv || !out_len) return NULL;

    size_t cap = 1024;
    size_t len = 0;
    char* buf = (char*)malloc(cap);
    if (!buf) return NULL;
    buf[0] = '\0';

    sk_key_value_child_t* child = kv->children;
    while (child) {
        sk_vdf_write_node(&buf, &cap, &len, child->node, 0);
        child = child->next;
    }

    *out_len = len;
    return buf;
}

static const char* sk_vdf_skip_whitespace(const char* p, const char* end) {
    while (p < end && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')) ++p;
    return p;
}

static const char* sk_vdf_parse_string(const char* p, const char* end, char* out_buf, size_t buf_size) {
    if (p >= end || *p != '"') return NULL;
    ++p;

    size_t i = 0;
    while (p < end && *p != '"') {
        if (*p == '\\' && p + 1 < end) {
            ++p;
            char c;
            switch (*p) {
                case '\\': c = '\\'; break;
                case '"': c = '"'; break;
                case 'n': c = '\n'; break;
                case 'r': c = '\r'; break;
                case 't': c = '\t'; break;
                default: c = *p; break;
            }
            if (i < buf_size - 1) out_buf[i++] = c;
            ++p;
        } else {
            if (i < buf_size - 1) out_buf[i++] = *p;
            ++p;
        }
    }
    if (p >= end) return NULL;
    out_buf[i] = '\0';
    return p + 1;
}

static const char* sk_vdf_parse_number(const char* p, const char* end, sk_kv_value_t* value) {
    char buf[64];
    size_t i = 0;
    while (p < end && i < sizeof(buf) - 1) {
        if ((*p >= '0' && *p <= '9') || *p == '-' || *p == '+' || *p == '.' || *p == 'x' || *p == 'X' ||
            (*p >= 'a' && *p <= 'f') || (*p >= 'A' && *p <= 'F')) {
            buf[i++] = *p++;
        } else {
            break;
        }
    }
    buf[i] = '\0';
    if (i == 0) return NULL;

    char* tail = NULL;
    if (strchr(buf, '.') || strchr(buf, 'e') || strchr(buf, 'E')) {
        double d = strtod(buf, &tail);
        if (tail == buf) return NULL;
        value->as.f64 = d;
        value->type = SK_KV_TYPE_FLOAT64;
    } else if (buf[0] == '-' || (buf[0] >= '0' && buf[0] <= '9')) {
        long long ll = strtoll(buf, &tail, 0);
        if (tail == buf) return NULL;
        if (ll >= INT32_MIN && ll <= INT32_MAX) {
            value->as.i32 = (int32_t)ll;
            value->type = SK_KV_TYPE_INT32;
        } else {
            value->as.i64 = ll;
            value->type = SK_KV_TYPE_INT64;
        }
    } else {
        return NULL;
    }
    return p;
}

static const char* sk_vdf_parse_bool(const char* p, const char* end, sk_kv_value_t* value) {
    if (p < end && (*p == '1' || *p == '0')) {
        value->as.b = (*p == '1');
        value->type = SK_KV_TYPE_BOOL_TRUE;
        return p + 1;
    }
    return NULL;
}

static const char* sk_vdf_parse_hex(const char* p, const char* end, sk_kv_value_t* value) {
    if (p + 2 >= end || p[0] != '0' || (p[1] != 'x' && p[1] != 'X')) return NULL;
    p += 2;
    uint32_t c = 0;
    size_t count = 0;
    while (p < end && count < 4) {
        char hex[3] = { *p, 0, 0 };
        char* tail = NULL;
        unsigned long digit = strtoul(hex, &tail, 16);
        if (*tail != 0) break;
        c = (c << 4) | (uint32_t)digit;
        ++p;
        ++count;
    }
    if (count == 4) {
        value->as.u32 = c;
        value->type = SK_KV_TYPE_COLOR;
        return p;
    }
    return NULL;
}

static const char* sk_vdf_parse_binary(const char* p, const char* end, sk_kv_value_t* value) {
    size_t len = end - p;
    if (len == 0 || len % 2 != 0) return NULL;
    size_t byte_len = len / 2;
    uint8_t* data = (uint8_t*)malloc(byte_len);
    if (!data) return NULL;
    for (size_t i = 0; i < byte_len; ++i) {
        char hex[3] = { p[i * 2], p[i * 2 + 1], 0 };
        char* tail = NULL;
        unsigned long byte = strtoul(hex, &tail, 16);
        if (*tail != 0) {
            free(data);
            return NULL;
        }
        data[i] = (uint8_t)byte;
    }
    value->as.binary.data = data;
    value->as.binary.len = byte_len;
    value->type = SK_KV_TYPE_BINARY_BLOB;
    return end;
}

static sk_key_value_t* sk_vdf_parse_node(const char** p, const char* end) {
    *p = sk_vdf_skip_whitespace(*p, end);
    if (*p >= end || **p != '"') return NULL;

    char name[256];
    *p = sk_vdf_parse_string(*p, end, name, sizeof(name));
    if (!*p) return NULL;

    sk_key_value_t* node = sk_key_value_create(name);
    if (!node) return NULL;

    *p = sk_vdf_skip_whitespace(*p, end);
    if (*p >= end) {
        sk_key_value_destroy(node);
        return NULL;
    }

    if (**p == '"') {
        char value[1024];
        *p = sk_vdf_parse_string(*p, end, value, sizeof(value));
        if (!*p) {
            sk_key_value_destroy(node);
            return NULL;
        }
        sk_key_value_set_string(node, value);
    } else if (**p == '{') {
        ++*p;
        node->value.type = SK_KV_TYPE_NONE;
        while (*p < end) {
            *p = sk_vdf_skip_whitespace(*p, end);
            if (*p >= end) break;
            if (**p == '}') {
                ++*p;
                break;
            }
            if (**p == '"') {
                sk_key_value_t* child = sk_vdf_parse_node(p, end);
                if (child) {
                    sk_key_value_add_child(node, child);
                }
            } else {
                ++*p;
            }
        }
    } else {
        const char* val_start = *p;
        sk_kv_type_t value_type = SK_KV_TYPE_NONE;

        if (**p == '0' && *(*p + 1) == 'x') {
            *p = sk_vdf_parse_hex(*p, end, &node->value);
            value_type = node->value.type;
        } else if ((**p >= '0' && **p <= '9') || **p == '-' || **p == '+') {
            *p = sk_vdf_parse_number(*p, end, &node->value);
            value_type = node->value.type;
        } else if (**p == '1' || **p == '0') {
            *p = sk_vdf_parse_bool(*p, end, &node->value);
            value_type = node->value.type;
        }

        if (value_type == SK_KV_TYPE_NONE) {
            *p = val_start;
            while (p < end && *p != '\n' && *p != '\r' && *p != '}' && *p != '"') ++p;
            size_t vlen = *p - val_start;
            char* vbuf = (char*)malloc(vlen + 1);
            if (vbuf) {
                memcpy(vbuf, val_start, vlen);
                vbuf[vlen] = '\0';
                sk_key_value_set_string(node, vbuf);
                free(vbuf);
            }
        }
    }

    return node;
}

sk_key_value_t* sk_key_value_parse_vdf(const char* text, size_t text_len) {
    if (!text || text_len == 0) return NULL;

    sk_key_value_t* root = sk_key_value_create("root");
    if (!root) return NULL;

    const char* p = text;
    const char* end = text + text_len;

    while (p < end) {
        p = sk_vdf_skip_whitespace(p, end);
        if (p >= end) break;
        if (*p == '"') {
            sk_key_value_t* child = sk_vdf_parse_node(&p, end);
            if (child) {
                sk_key_value_add_child(root, child);
            }
        } else {
            ++p;
        }
    }

    return root;
}

sk_key_value_t* sk_key_value_load_vdf_file(const char* path) {
    if (!path) return NULL;

    FILE* f = fopen(path, "rb");
    if (!f) return NULL;

    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (fsize <= 0) {
        fclose(f);
        return NULL;
    }

    char* buf = (char*)malloc((size_t)fsize + 1);
    if (!buf) {
        fclose(f);
        return NULL;
    }

    size_t read = fread(buf, 1, (size_t)fsize, f);
    fclose(f);
    buf[read] = '\0';

    sk_key_value_t* kv = sk_key_value_parse_vdf(buf, read);
    free(buf);
    return kv;
}

bool sk_key_value_save_vdf_file(const sk_key_value_t* kv, const char* path) {
    if (!kv || !path) return false;

    size_t len = 0;
    char* vdf = sk_key_value_write_vdf(kv, &len);
    if (!vdf) return false;

    FILE* f = fopen(path, "wb");
    if (!f) {
        free(vdf);
        return false;
    }

    fwrite(vdf, 1, len, f);
    fclose(f);
    free(vdf);
    return true;
}
