#ifndef STEAMKIT_BASE_GENERATED_STEAM_MSG_GC_H
#define STEAMKIT_BASE_GENERATED_STEAM_MSG_GC_H

#include <stdint.h>
#include <stdbool.h>
#include "steamkit/base/emsg.h"

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(push, 1)

// CMsgClientHello
typedef struct sk_cmsg_client_hello {
    uint32_t version;
} sk_cmsg_client_hello_t;

// CMsgClientWelcome
typedef struct sk_cmsg_client_welcome {
    uint32_t result;
} sk_cmsg_client_welcome_t;

// CMsgGCClientHello
typedef struct sk_cmsg_gc_client_hello {
    uint32_t app_id;
    uint32_t version;
} sk_cmsg_gc_client_hello_t;

// CMsgGCClientWelcome
typedef struct sk_cmsg_gc_client_welcome {
    uint32_t result;
} sk_cmsg_gc_client_welcome_t;

#pragma pack(pop)

#ifdef __cplusplus
}
#endif

#endif // STEAMKIT_BASE_GENERATED_STEAM_MSG_GC_H
