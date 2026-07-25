#include "steamkit/base/emsg.h"

const char* sk_emsg_to_string(sk_emsg_t msg) {
    // Strip protobuf mask before matching
    uint32_t raw = (uint32_t)msg & ~(uint32_t)SK_EMSG_PROTO_MASK;
    switch ((sk_emsg_t)raw) {
        case SK_EMSG_INVALID:                                     return "Invalid";
        case SK_EMSG_MULTI:                                       return "Multi";
        case SK_EMSG_SERVICE_METHOD:                              return "ServiceMethod";
        case SK_EMSG_SERVICE_METHOD_RESPONSE:                     return "ServiceMethodResponse";
        case SK_EMSG_SERVICE_METHOD_CALL_FROM_CLIENT:             return "ServiceMethodCallFromClient";
        case SK_EMSG_SERVICE_METHOD_CALL_FROM_CLIENT_NON_AUTHED:  return "ServiceMethodCallFromClientNonAuthed";
        case SK_EMSG_CHANNEL_ENCRYPT_REQUEST:                     return "ChannelEncryptRequest";
        case SK_EMSG_CHANNEL_ENCRYPT_RESPONSE:                    return "ChannelEncryptResponse";
        case SK_EMSG_CHANNEL_ENCRYPT_RESULT:                      return "ChannelEncryptResult";
        case SK_EMSG_DISCONNECT_NOTIFY:                           return "DisconnectNotify";
        case SK_EMSG_CLIENT_HEARTBEAT:                            return "ClientHeartBeat";
        case SK_EMSG_CLIENT_LOG_OFF:                              return "ClientLogOff";
        case SK_EMSG_CLIENT_REMOVE_FRIEND:                        return "ClientRemoveFriend";
        case SK_EMSG_CLIENT_LOGON_RESPONSE:                       return "ClientLogOnResponse";
        case SK_EMSG_CLIENT_LOGGED_OFF:                           return "ClientLoggedOff";
        case SK_EMSG_CLIENT_SESSION_TOKEN:                        return "ClientSessionToken";
        case SK_EMSG_CLIENT_PERSONA_STATE:                        return "ClientPersonaState";
        case SK_EMSG_CLIENT_FRIENDS_LIST:                         return "ClientFriendsList";
        case SK_EMSG_CLIENT_ACCOUNT_INFO:                         return "ClientAccountInfo";
        case SK_EMSG_CLIENT_LICENSE_LIST:                         return "ClientLicenseList";
        case SK_EMSG_CLIENT_VAC_BAN_STATUS:                       return "ClientVACBanStatus";
        case SK_EMSG_CLIENT_ADD_FRIEND:                           return "ClientAddFriend";
        case SK_EMSG_CLIENT_CHAT_MSG:                             return "ClientChatMsg";
        case SK_EMSG_CLIENT_CHAT_INVITE:                          return "ClientChatInvite";
        case SK_EMSG_CLIENT_JOIN_CHAT:                            return "ClientJoinChat";
        case SK_EMSG_CLIENT_CHAT_MEMBER_INFO:                     return "ClientChatMemberInfo";
        case SK_EMSG_CLIENT_GET_APP_OWNERSHIP_TICKET:             return "ClientGetAppOwnershipTicket";
        case SK_EMSG_CLIENT_GET_APP_OWNERSHIP_TICKET_RESPONSE:    return "ClientGetAppOwnershipTicketResponse";
        case SK_EMSG_CLIENT_REQUEST_FRIEND_DATA:                  return "ClientRequestFriendData";
        case SK_EMSG_CLIENT_GET_DEPOT_DECRYPTION_KEY:             return "ClientGetDepotDecryptionKey";
        case SK_EMSG_CLIENT_GET_DEPOT_DECRYPTION_KEY_RESPONSE:    return "ClientGetDepotDecryptionKeyResponse";
        case SK_EMSG_CLIENT_TO_GC:                                return "ClientToGC";
        case SK_EMSG_CLIENT_FROM_GC:                              return "ClientFromGC";
        case SK_EMSG_CLIENT_LOG_ON:                               return "ClientLogon";
        case SK_EMSG_CLIENT_FRIENDS_GROUPS_LIST:                  return "ClientFriendsGroupsList";
        case SK_EMSG_CLIENT_REQUEST_FREE_LICENSE:                 return "ClientRequestFreeLicense";
        case SK_EMSG_CLIENT_REQUEST_FREE_LICENSE_RESPONSE:        return "ClientRequestFreeLicenseResponse";
        case SK_EMSG_CLIENT_PICS_CHANGES_SINCE_REQUEST:           return "ClientPICSChangesSinceRequest";
        case SK_EMSG_CLIENT_PICS_CHANGES_SINCE_RESPONSE:          return "ClientPICSChangesSinceResponse";
        case SK_EMSG_CLIENT_PICS_PRODUCT_INFO_REQUEST:            return "ClientPICSProductInfoRequest";
        case SK_EMSG_CLIENT_PICS_PRODUCT_INFO_RESPONSE:           return "ClientPICSProductInfoResponse";
        case SK_EMSG_CLIENT_PICS_ACCESS_TOKEN_REQUEST:            return "ClientPICSAccessTokenRequest";
        case SK_EMSG_CLIENT_PICS_ACCESS_TOKEN_RESPONSE:           return "ClientPICSAccessTokenResponse";
        case SK_EMSG_CLIENT_PICS_PRIVATE_BETA_REQUEST:            return "ClientPICSPrivateBetaRequest";
        case SK_EMSG_CLIENT_PICS_PRIVATE_BETA_RESPONSE:           return "ClientPICSPrivateBetaResponse";
        default:                                                  return "Unknown";
    }
}
