#include "steamkit/types/global_id.h"
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

sk_global_id_t* sk_global_id_create(uint64_t value) {
    sk_global_id_t* id = (sk_global_id_t*)malloc(sizeof(sk_global_id_t));
    if (id) {
        id->value = value;
    }
    return id;
}

sk_global_id_t* sk_global_id_create_invalid(void) {
    return sk_global_id_create(0);
}

uint64_t sk_global_id_value(const sk_global_id_t* id) {
    return id ? id->value : 0;
}

bool sk_global_id_equals(const sk_global_id_t* a, const sk_global_id_t* b) {
    if (!a && !b) return true;
    if (!a || !b) return false;
    return a->value == b->value;
}

void sk_global_id_destroy(sk_global_id_t* id) {
    free(id);
}
