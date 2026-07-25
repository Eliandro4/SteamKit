#ifndef STEAMKIT_TYPES_GLOBAL_ID_H
#define STEAMKIT_TYPES_GLOBAL_ID_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sk_global_id {
    uint64_t value;
} sk_global_id_t;

// Creates a new GlobalID
sk_global_id_t* sk_global_id_create(uint64_t value);

// Creates an invalid GlobalID
sk_global_id_t* sk_global_id_create_invalid(void);

// Gets the raw value
uint64_t sk_global_id_value(const sk_global_id_t* id);

// Equality check
bool sk_global_id_equals(const sk_global_id_t* a, const sk_global_id_t* b);

// Destroys a GlobalID
void sk_global_id_destroy(sk_global_id_t* id);

#ifdef __cplusplus
}
#endif

#endif // STEAMKIT_TYPES_GLOBAL_ID_H
