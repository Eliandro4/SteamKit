#ifndef STEAMKIT_UTILS_LANCACHE_H
#define STEAMKIT_UTILS_LANCACHE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sk_lancache_info {
    char* host;
    uint16_t port;
    bool is_lancache;
} sk_lancache_info_t;

bool sk_lancache_detect(const char* host, uint16_t port);
sk_lancache_info_t* sk_lancache_probe(const char* host, uint16_t port);
void sk_lancache_info_destroy(sk_lancache_info_t* info);

#ifdef __cplusplus
}
#endif

#endif // STEAMKIT_UTILS_LANCACHE_H