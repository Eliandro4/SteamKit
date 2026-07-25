#ifndef STEAMKIT_BASE_GENERATED_STEAM_MSG_LOBBY_H
#define STEAMKIT_BASE_GENERATED_STEAM_MSG_LOBBY_H

#include <stdint.h>
#include <stdbool.h>
#include "steamkit/base/emsg.h"
#include "steamkit/types/steam_id.h"

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(push, 1)

// CMsgLobbyInvite
typedef struct sk_cmsg_lobby_invite {
    uint64_t steam_id_inviter;
    uint64_t steam_id_lobby;
    uint32_t lobby_type;
} sk_cmsg_lobby_invite_t;

// CMsgLobbyJoin
typedef struct sk_cmsg_lobby_join {
    uint64_t steam_id_lobby;
    uint64_t steam_id_invited;
    uint32_t lobby_type;
    uint32_t response;
} sk_cmsg_lobby_join_t;

// CMsgLobbyLeave
typedef struct sk_cmsg_lobby_leave {
    uint64_t steam_id_lobby;
    uint32_t lobby_type;
} sk_cmsg_lobby_leave_t;

// CMsgLobbyUpdate
typedef struct sk_cmsg_lobby_update {
    uint64_t steam_id_lobby;
    uint32_t lobby_type;
} sk_cmsg_lobby_update_t;

// CMsgLobbyChatMsg
typedef struct sk_cmsg_lobby_chat_msg {
    uint64_t steam_id_chat;
    uint32_t chatroom_type;
    uint64_t steam_id_sender;
    char message[1];
} sk_cmsg_lobby_chat_msg_t;

// CMsgLobbyChatEnter
typedef struct sk_cmsg_lobby_chat_enter {
    uint64_t steam_id_lobby;
    uint32_t chatroom_type;
    uint64_t steam_id_owner;
    uint32_t response;
} sk_cmsg_lobby_chat_enter_t;

// CMsgLobbyChatLeave
typedef struct sk_cmsg_lobby_chat_leave {
    uint64_t steam_id_lobby;
    uint32_t chatroom_type;
} sk_cmsg_lobby_chat_leave_t;

#pragma pack(pop)

#ifdef __cplusplus
}
#endif

#endif // STEAMKIT_BASE_GENERATED_STEAM_MSG_LOBBY_H
