#include "steamkit/types/job_id.h"
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

sk_job_id_t* sk_job_id_create(uint64_t job_id) {
    sk_job_id_t* id = (sk_job_id_t*)malloc(sizeof(sk_job_id_t));
    if (id) {
        id->base.value = job_id;
    }
    return id;
}

sk_job_id_t* sk_job_id_create_invalid(void) {
    return sk_job_id_create(0);
}

bool sk_job_id_is_valid(const sk_job_id_t* job_id) {
    return job_id && job_id->base.value != 0;
}

uint64_t sk_job_id_value(const sk_job_id_t* job_id) {
    return job_id ? job_id->base.value : 0;
}

bool sk_job_id_equals(const sk_job_id_t* a, const sk_job_id_t* b) {
    if (!a && !b) return true;
    if (!a || !b) return false;
    return a->base.value == b->base.value;
}

void sk_job_id_destroy(sk_job_id_t* job_id) {
    free(job_id);
}
