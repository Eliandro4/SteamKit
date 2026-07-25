#ifndef STEAMKIT_BASE_EMSG_H
#define STEAMKIT_BASE_EMSG_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// EMsg values taken directly from SteamKit2 (C#) / SteamDatabase Protobufs.
// These are the real Steam network protocol message type identifiers.
typedef enum sk_emsg {
    SK_EMSG_INVALID                                     = 0,
    SK_EMSG_MULTI                                       = 1,

    // Service method calls (unified messaging)
    SK_EMSG_SERVICE_METHOD                              = 146,
    SK_EMSG_SERVICE_METHOD_RESPONSE                     = 147,
    SK_EMSG_SERVICE_METHOD_CALL_FROM_CLIENT             = 151,
    SK_EMSG_SERVICE_METHOD_CALL_FROM_CLIENT_NON_AUTHED  = 9804,

    // Connection / session
    SK_EMSG_CHANNEL_ENCRYPT_REQUEST                     = 0x40000008,
    SK_EMSG_CHANNEL_ENCRYPT_RESPONSE                    = 0x40000009,
    SK_EMSG_CHANNEL_ENCRYPT_RESULT                      = 0x4000000A,
    SK_EMSG_DISCONNECT_NOTIFY                           = 0x40000002,

    // Client session
    SK_EMSG_CLIENT_HEARTBEAT                            = 703,
    SK_EMSG_CLIENT_LOG_OFF                              = 706,
    SK_EMSG_CLIENT_REMOVE_FRIEND                        = 714,

    // Login / logon
    SK_EMSG_CLIENT_LOGON_RESPONSE                       = 751,  // EMsg.ClientLogOnResponse (old name)
    SK_EMSG_CLIENT_LOGGED_OFF                           = 757,
    SK_EMSG_CLIENT_SESSION_TOKEN                        = 850,

    // Friends / personas
    SK_EMSG_CLIENT_PERSONA_STATE                        = 766,
    SK_EMSG_CLIENT_FRIENDS_LIST                         = 767,
    SK_EMSG_CLIENT_ACCOUNT_INFO                         = 768,
    SK_EMSG_CLIENT_LICENSE_LIST                         = 780,
    SK_EMSG_CLIENT_VAC_BAN_STATUS                       = 782,
    SK_EMSG_CLIENT_ADD_FRIEND                           = 791,

    // Old Steam chat
    SK_EMSG_CLIENT_CHAT_MSG                             = 799,
    SK_EMSG_CLIENT_CHAT_INVITE                          = 800,
    SK_EMSG_CLIENT_JOIN_CHAT                            = 801,
    SK_EMSG_CLIENT_CHAT_MEMBER_INFO                     = 802,

    // App ownership tickets
    SK_EMSG_CLIENT_GET_APP_OWNERSHIP_TICKET             = 857,
    SK_EMSG_CLIENT_GET_APP_OWNERSHIP_TICKET_RESPONSE    = 858,

    SK_EMSG_CLIENT_REQUEST_FRIEND_DATA                  = 815,

    // Content / depot
    SK_EMSG_CLIENT_GET_DEPOT_DECRYPTION_KEY             = 5438,
    SK_EMSG_CLIENT_GET_DEPOT_DECRYPTION_KEY_RESPONSE    = 5439,

    // Game Coordinator
    SK_EMSG_CLIENT_TO_GC                               = 5452,
    SK_EMSG_CLIENT_FROM_GC                             = 5453,

    // Logon (protobuf)
    SK_EMSG_CLIENT_LOG_ON                               = 5514,  // EMsg.ClientLogon

    // Friends groups
    SK_EMSG_CLIENT_FRIENDS_GROUPS_LIST                  = 5553,

    // Free license
    SK_EMSG_CLIENT_REQUEST_FREE_LICENSE                 = 5572,
    SK_EMSG_CLIENT_REQUEST_FREE_LICENSE_RESPONSE        = 5573,

    // User stats / leaderboards (SteamKit2: SteamUserStats handler)
    SK_EMSG_CLIENT_LBS_FIND_OR_CREATE_LB                = 5416,
    SK_EMSG_CLIENT_LBS_FIND_OR_CREATE_LB_RESPONSE       = 5417,
    SK_EMSG_CLIENT_LBS_GET_LB_ENTRIES                   = 5418,
    SK_EMSG_CLIENT_LBS_GET_LB_ENTRIES_RESPONSE          = 5419,
    SK_EMSG_CLIENT_GET_NUMBER_OF_CURRENT_PLAYERS_DP     = 5592,
    SK_EMSG_CLIENT_GET_NUMBER_OF_CURRENT_PLAYERS_DP_RESPONSE = 5593,

    // PICS (modern app/package info)
    SK_EMSG_CLIENT_PICS_CHANGES_SINCE_REQUEST           = 8901,
    SK_EMSG_CLIENT_PICS_CHANGES_SINCE_RESPONSE          = 8902,
    SK_EMSG_CLIENT_PICS_PRODUCT_INFO_REQUEST            = 8903,
    SK_EMSG_CLIENT_PICS_PRODUCT_INFO_RESPONSE           = 8904,
    SK_EMSG_CLIENT_PICS_ACCESS_TOKEN_REQUEST            = 8905,
    SK_EMSG_CLIENT_PICS_ACCESS_TOKEN_RESPONSE           = 8906,
    SK_EMSG_CLIENT_PICS_PRIVATE_BETA_REQUEST            = 8907,
    SK_EMSG_CLIENT_PICS_PRIVATE_BETA_RESPONSE           = 8908,

    // Bit masks
    SK_EMSG_PROTO_MASK                                  = 0x80000000,
} sk_emsg_t;

static inline bool sk_emsg_is_proto(sk_emsg_t msg) {
    return ((uint32_t)msg & SK_EMSG_PROTO_MASK) != 0;
}

static inline bool sk_emsg_is_client_to_gc(sk_emsg_t msg) {
    return msg == SK_EMSG_CLIENT_TO_GC;
}

static inline bool sk_emsg_is_client_to_server(sk_emsg_t msg) {
    // Covers login, friends, content, PICS ranges
    return (msg >= 700 && msg <= 899) ||
           (msg >= 5400 && msg <= 5599) ||
           (msg >= 8900 && msg <= 8999);
}

static inline bool sk_emsg_is_server_to_client(sk_emsg_t msg) {
    return (msg >= 750 && msg <= 860) ||
           (msg >= 5439 && msg <= 5439) ||
           (msg >= 8902 && msg <= 8908);
}

const char* sk_emsg_to_string(sk_emsg_t msg);

#ifdef __cplusplus
}
#endif

#endif // STEAMKIT_BASE_EMSG_H
