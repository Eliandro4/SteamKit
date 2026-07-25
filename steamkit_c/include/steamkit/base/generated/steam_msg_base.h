#ifndef STEAMKIT_BASE_GENERATED_STEAM_MSG_BASE_H
#define STEAMKIT_BASE_GENERATED_STEAM_MSG_BASE_H

#include <stdint.h>
#include <stdbool.h>
#include "steamkit/base/emsg.h"
#include "steamkit/types/steam_id.h"

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

// CMsgClientFriendsList
typedef struct sk_cmsg_client_friends_list {
    uint32_t incremented;
    uint32_t max_friends;
} sk_cmsg_client_friends_list_t;

// CMsgClientPersonaState
typedef struct sk_cmsg_client_persona_state {
    uint32_t count;
} sk_cmsg_client_persona_state_t;

// CMsgClientChatInvite
typedef struct sk_cmsg_client_chat_invite {
    uint64_t steam_id_invited;
    uint64_t steam_id_patron;
    uint64_t steam_id_chat;
    uint32_t chatroom_type;
    char chat_name[1];
} sk_cmsg_client_chat_invite_t;

// CMsgClientChatJoin
typedef struct sk_cmsg_client_chat_join {
    uint64_t steam_id_chat;
    uint32_t chatroom_type;
    char chat_name[1];
    uint32_t response;
} sk_cmsg_client_chat_join_t;

// CMsgClientChatLeave
typedef struct sk_cmsg_client_chat_leave {
    uint64_t steam_id_chat;
    uint32_t chatroom_type;
    uint32_t response;
} sk_cmsg_client_chat_leave_t;

// CMsgClientChatMemberInfo
typedef struct sk_cmsg_client_chat_member_info {
    uint64_t steam_id_chat;
    uint32_t chatroom_type;
    uint32_t action;
} sk_cmsg_client_chat_member_info_t;

// CMsgClientChatKick
typedef struct sk_cmsg_client_chat_kick {
    uint64_t steam_id_chat;
    uint32_t chatroom_type;
    uint64_t steam_id_kicked;
} sk_cmsg_client_chat_kick_t;

#pragma pack(pop)

#ifdef __cplusplus
}
#endif

#endif // STEAMKIT_BASE_GENERATED_STEAM_MSG_BASE_H
