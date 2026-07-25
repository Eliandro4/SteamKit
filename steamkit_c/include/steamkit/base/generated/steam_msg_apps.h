#ifndef STEAMKIT_BASE_GENERATED_STEAM_MSG_APPS_H
#define STEAMKIT_BASE_GENERATED_STEAM_MSG_APPS_H

#include <stdint.h>
#include <stdbool.h>
#include "steamkit/base/emsg.h"

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(push, 1)

// CMsgClientAppInfo
typedef struct sk_cmsg_client_app_info {
    uint32_t app_id;
    uint32_t result;
} sk_cmsg_client_app_info_t;

// CMsgClientPackageInfo
typedef struct sk_cmsg_client_package_info {
    uint32_t package_id;
    uint32_t result;
} sk_cmsg_client_package_info_t;

// CMsgClientAppInfoRequest
typedef struct sk_cmsg_client_app_info_request {
    uint32_t app_id;
} sk_cmsg_client_app_info_request_t;

// CMsgClientPackageInfoRequest
typedef struct sk_cmsg_client_package_info_request {
    uint32_t package_id;
} sk_cmsg_client_package_info_request_t;

// CMsgClientNewApps
typedef struct sk_cmsg_client_new_apps {
    uint32_t count;
} sk_cmsg_client_new_apps_t;

// CMsgClientAppsComplete
typedef struct sk_cmsg_client_apps_complete {
    uint32_t result;
} sk_cmsg_client_apps_complete_t;

#pragma pack(pop)

#ifdef __cplusplus
}
#endif

#endif // STEAMKIT_BASE_GENERATED_STEAM_MSG_APPS_H
