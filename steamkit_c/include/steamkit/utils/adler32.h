#ifndef STEAMKIT_UTILS_ADLER32_H
#define STEAMKIT_UTILS_ADLER32_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

uint32_t sk_adler32(const uint8_t* data, size_t len);

#ifdef __cplusplus
}
#endif

#endif // STEAMKIT_UTILS_ADLER32_H