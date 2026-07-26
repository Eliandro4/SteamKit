#include "steamkit/utils/adler32.h"
#include <stdint.h>
#include <stddef.h>

#define ADLER32_MOD 65521

uint32_t sk_adler32(const uint8_t* data, size_t len) {
    uint32_t a = 1;
    uint32_t b = 0;

    for (size_t i = 0; i < len; ++i) {
        a = (a + (uint32_t)data[i]) % ADLER32_MOD;
        b = (b + a) % ADLER32_MOD;
    }

    return (b << 16) | a;
}