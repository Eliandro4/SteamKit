#ifndef STEAMKIT_TYPES_MSG_OBJECT_H
#define STEAMKIT_TYPES_MSG_OBJECT_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Message object - mirrors C# MessageObject
typedef struct sk_msg_object sk_msg_object_t;

sk_msg_object_t* sk_msg_object_create(void);
void sk_msg_object_destroy(sk_msg_object_t* obj);
sk_msg_object_t* sk_msg_object_clone(const sk_msg_object_t* obj);

const char* sk_msg_object_type_name(const sk_msg_object_t* obj);
void sk_msg_object_set_type_name(sk_msg_object_t* obj, const char* type_name);
uint32_t sk_msg_object_size(const sk_msg_object_t* obj);
void sk_msg_object_set_size(sk_msg_object_t* obj, uint32_t size);

#ifdef __cplusplus
}
#endif

#endif // STEAMKIT_TYPES_MSG_OBJECT_H
