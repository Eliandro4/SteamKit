#ifndef STEAMKIT_BASE_GENERATED_STEAM_MSG_FRIENDS_H
#define STEAMKIT_BASE_GENERATED_STEAM_MSG_FRIENDS_H

#include <stdint.h>
#include <stdbool.h>
#include "steamkit/base/emsg.h"
#include "steamkit/types/steam_id.h"

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(push, 1)

// CMsgClientFriendsList
typedef struct sk_cmsg_client_friends_list {
    uint32_t incremented;
    uint32_t max_friends;
} sk_cmsg_client_friends_list_t;

// CMsgClientPersonaState - friend info
typedef struct sk_cmsg_persona_state_friend {
    uint64_t steam_id;
    uint32_t persona_state;
    uint32_t game_id_app;
    uint32_t game_id_mod;
    uint32_t persona_state_flags;
    char persona_name[1];
} sk_cmsg_persona_state_friend_t;

// CMsgClientPersonaState
typedef struct sk_cmsg_client_persona_state {
    uint32_t count;
} sk_cmsg_client_persona_state_t;

// CMsgClientNicknameList
typedef struct sk_cmsg_client_nickname_list {
    uint32_t count;
} sk_cmsg_client_nickname_list_t;

// CMsgGenericResult
typedef struct sk_cmsg_generic_result {
    uint32_t result;
    char extended_result[1];
} sk_cmsg_generic_result_t;

#pragma pack(pop)

#ifdef __cplusplus
}
#endif

#endif // STEAMKIT_BASE_GENERATED_STEAM_MSG_FRIENDS_H
