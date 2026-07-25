#ifndef SK2_NATIVE_H
#define SK2_NATIVE_H

#include <stdint.h>
#include "dnne.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void* SK2_Client;

typedef enum {
    SK2_Callback_Unknown = 0,
    SK2_Callback_Connected = 1,
    SK2_Callback_Disconnected = 2,
    SK2_Callback_LoggedOn = 3,
    SK2_Callback_LoggedOff = 4,
    SK2_Callback_AccountInfo = 5,
    SK2_Callback_PersonaState = 6,
    SK2_Callback_FriendsList = 7,
    SK2_Callback_FriendAdded = 8,
    SK2_Callback_FriendMsg = 9,
    SK2_Callback_WalletInfo = 10,
    SK2_Callback_SessionToken = 11
} SK2_CallbackType;

SK2_Client SK2_CreateClient(void);
void SK2_FreeClient(SK2_Client client);

void SK2_Connect(SK2_Client client);
void SK2_Disconnect(SK2_Client client);
int SK2_IsConnected(SK2_Client client);

int SK2_Tick(SK2_Client client, int timeoutMs);
int SK2_TickNoWait(SK2_Client client);
int SK2_GetCallbackCount(SK2_Client client);
uint32_t SK2_GetCallbackType(SK2_Client client, int index);
uint64_t SK2_GetCallbackSteamID(SK2_Client client, int index);
uint64_t SK2_GetCallbackJobID(SK2_Client client, int index);
int32_t SK2_GetCallbackResult(SK2_Client client, int index);
const char* SK2_GetCallbackString(SK2_Client client, int index);

void SK2_LogOn(SK2_Client client, const char* accountName, const char* password);
void SK2_LogOnAnon(SK2_Client client);
void SK2_LogOff(SK2_Client client);

void SK2_SetPersonaState(SK2_Client client, uint32_t state);
void SK2_SetPersonaName(SK2_Client client, const char* name);
const char* SK2_GetPersonaName(SK2_Client client);

int SK2_GetFriendCount(SK2_Client client);
uint64_t SK2_GetFriendByIndex(SK2_Client client, int index);
const char* SK2_GetFriendPersonaName(SK2_Client client, uint64_t steamId);
uint32_t SK2_GetFriendPersonaState(SK2_Client client, uint64_t steamId);
uint32_t SK2_GetFriendRelationship(SK2_Client client, uint64_t steamId);
void SK2_AddFriend(SK2_Client client, const char* accountNameOrEmail);
void SK2_RemoveFriend(SK2_Client client, uint64_t steamId);

void SK2_FreeString(const char* str);

#ifdef __cplusplus
}
#endif

#endif