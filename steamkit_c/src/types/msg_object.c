#include "steamkit/types/msg_object.h"
#include <stdlib.h>
#include <string.h>

static char* sk_strdup(const char* s) {
    if (!s) return NULL;
    size_t len = strlen(s) + 1;
    char* dup = (char*)malloc(len);
    if (dup) memcpy(dup, s, len);
    return dup;
}

struct sk_msg_object {
    char* type_name;
    uint32_t size;
};

sk_msg_object_t* sk_msg_object_create(void) {
    return (sk_msg_object_t*)calloc(1, sizeof(sk_msg_object_t));
}

void sk_msg_object_destroy(sk_msg_object_t* obj) {
    if (!obj) return;
    free(obj->type_name);
    free(obj);
}

sk_msg_object_t* sk_msg_object_clone(const sk_msg_object_t* obj) {
    if (!obj) return NULL;
    sk_msg_object_t* clone = sk_msg_object_create();
    if (!clone) return NULL;
    if (obj->type_name) {
        clone->type_name = sk_strdup(obj->type_name);
    }
    clone->size = obj->size;
    return clone;
}

const char* sk_msg_object_type_name(const sk_msg_object_t* obj) {
    return obj ? obj->type_name : NULL;
}

void sk_msg_object_set_type_name(sk_msg_object_t* obj, const char* type_name) {
    if (!obj) return;
    free(obj->type_name);
        obj->type_name = type_name ? sk_strdup(type_name) : NULL;
}

uint32_t sk_msg_object_size(const sk_msg_object_t* obj) {
    return obj ? obj->size : 0;
}

void sk_msg_object_set_size(sk_msg_object_t* obj, uint32_t size) {
    if (obj) obj->size = size;
}
