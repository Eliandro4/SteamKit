#ifndef STEAMKIT_BASE_GENERATED_STEAM_MSG_USER_H
#define STEAMKIT_BASE_GENERATED_STEAM_MSG_USER_H

#include <stdint.h>
#include <stdbool.h>
#include "steamkit/base/emsg.h"

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(push, 1)

// CMsgClientLogon
typedef struct sk_cmsg_client_logon {
    uint32_t protocol_version;
    uint32_t deprecated_1;
    uint32_t deprecated_2;
    uint64_t cell_id;
    char client_language[4];
    uint32_t os_version;
    uint32_t deprecated_3;
    char game_dir[4];
    char game_primary_language[4];
    char game_secondary_language[4];
} sk_cmsg_client_logon_t;

// CMsgClientLogOff
typedef struct sk_cmsg_client_log_off {
    uint32_t deprecated;
} sk_cmsg_client_log_off_t;

// CMsgClientAccountInfo
typedef struct sk_cmsg_client_account_info {
    char persona_name[1];
} sk_cmsg_client_account_info_t;

// CMsgClientRequestFriendData
typedef struct sk_cmsg_client_request_friend_data {
    uint32_t persona_state_flags;
    uint64_t steam_id_friend;
} sk_cmsg_client_request_friend_data_t;

// CMsgClientChangePersonaState
typedef struct sk_cmsg_client_change_persona_state {
    uint32_t persona_state;
} sk_cmsg_client_change_persona_state_t;

#pragma pack(pop)

#ifdef __cplusplus
}
#endif

#endif // STEAMKIT_BASE_GENERATED_STEAM_MSG_USER_H
