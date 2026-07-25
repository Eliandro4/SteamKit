#ifndef STEAMKIT_TYPES_JOB_ID_H
#define STEAMKIT_TYPES_JOB_ID_H

#include <stdint.h>
#include <stdbool.h>
#include "steamkit/types/global_id.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sk_job_id {
    sk_global_id_t base;
} sk_job_id_t;

// Creates a new JobID from a uint64_t value
sk_job_id_t* sk_job_id_create(uint64_t job_id);

// Creates an invalid JobID
sk_job_id_t* sk_job_id_create_invalid(void);

// Checks if this JobID is valid
bool sk_job_id_is_valid(const sk_job_id_t* job_id);

// Gets the raw 64-bit value
uint64_t sk_job_id_value(const sk_job_id_t* job_id);

// Equality check
bool sk_job_id_equals(const sk_job_id_t* a, const sk_job_id_t* b);

// Destroys a JobID
void sk_job_id_destroy(sk_job_id_t* job_id);

#ifdef __cplusplus
}
#endif

#endif // STEAMKIT_TYPES_JOB_ID_H
