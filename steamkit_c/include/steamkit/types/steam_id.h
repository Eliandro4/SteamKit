#ifndef STEAMKIT_TYPES_STEAM_ID_H
#define STEAMKIT_TYPES_STEAM_ID_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// EAccountType mirrors C# SteamID.EAccountType
typedef enum sk_account_type {
    SK_ACCOUNT_TYPE_INVALID = 0,
    SK_ACCOUNT_TYPE_INDIVIDUAL = 1,
    SK_ACCOUNT_TYPE_MULTISEAT = 2,
    SK_ACCOUNT_TYPE_GAME_SERVER = 3,
    SK_ACCOUNT_TYPE_ANON_GAME_SERVER = 4,
    SK_ACCOUNT_TYPE_PENDING = 5,
    SK_ACCOUNT_TYPE_CONTENT_SERVER = 6,
    SK_ACCOUNT_TYPE_CLAN = 7,
    SK_ACCOUNT_TYPE_CHAT = 8,
    SK_ACCOUNT_TYPE_CONSOLE_USER = 9,
    SK_ACCOUNT_TYPE_ANON_USER = 10,
    SK_ACCOUNT_TYPE_MAX = 11
} sk_account_type_t;

// EUniverse mirrors C# SteamID.EUniverse
typedef enum sk_universe {
    SK_UNIVERSE_INVALID = 0,
    SK_UNIVERSE_PUBLIC = 1,
    SK_UNIVERSE_BETA = 2,
    SK_UNIVERSE_INTERNAL = 3,
    SK_UNIVERSE_DEV = 4,
    SK_UNIVERSE_MAX = 5
} sk_universe_t;

// SteamID structure - mirrors C# SteamID
typedef struct sk_steam_id {
    uint64_t steamid;
} sk_steam_id_t;

// Creates a new SteamID from a 64-bit value
sk_steam_id_t* sk_steam_id_create(uint64_t steamid);

// Creates a new SteamID from components
sk_steam_id_t* sk_steam_id_create_account(sk_universe_t universe, sk_account_type_t type, 
                                           uint32_t account_id, uint32_t instance);

// Destroys a SteamID
void sk_steam_id_destroy(sk_steam_id_t* sid);

// Gets the 64-bit value
uint64_t sk_steam_id_to_uint64(const sk_steam_id_t* sid);

// Gets account ID
uint32_t sk_steam_id_account_id(const sk_steam_id_t* sid);

// Gets instance
uint32_t sk_steam_id_instance(const sk_steam_id_t* sid);

// Gets account type
sk_account_type_t sk_steam_id_type(const sk_steam_id_t* sid);

// Gets universe
sk_universe_t sk_steam_id_universe(const sk_steam_id_t* sid);

// Checks if this is a clan
bool sk_steam_id_is_clan(const sk_steam_id_t* sid);

// Checks if this is an individual user
bool sk_steam_id_is_individual(const sk_steam_id_t* sid);

// Validity checks
bool sk_steam_id_is_valid(const sk_steam_id_t* sid);
bool sk_steam_id_is_anon(const sk_steam_id_t* sid);
bool sk_steam_id_is_game_server(const sk_steam_id_t* sid);
bool sk_steam_id_is_content_server(const sk_steam_id_t* sid);
bool sk_steam_id_is_pending(const sk_steam_id_t* sid);
bool sk_steam_id_is_chat(const sk_steam_id_t* sid);
bool sk_steam_id_is_console_user(const sk_steam_id_t* sid);
bool sk_steam_id_is_unknown(const sk_steam_id_t* sid);
bool sk_steam_id_is_group(const sk_steam_id_t* sid);
bool sk_steam_id_is_lobby(const sk_steam_id_t* sid);

// Static account key (does not include instance)
uint64_t sk_steam_id_static_account_key(const sk_steam_id_t* sid);

// Clone / set helpers
sk_steam_id_t* sk_steam_id_clone(const sk_steam_id_t* sid);
void sk_steam_id_set_from_uint64(sk_steam_id_t* sid, uint64_t steamid);

// Equality check
bool sk_steam_id_equals(const sk_steam_id_t* a, const sk_steam_id_t* b);

// String representation (caller must free)
char* sk_steam_id_to_string(const sk_steam_id_t* sid);

// Create from Steam2 string (STEAM_X:Y:Z)
sk_steam_id_t* sk_steam_id_from_steam2(const char* steam2, sk_universe_t universe);

// Create from Steam3 string ([X:Y:Z])
sk_steam_id_t* sk_steam_id_from_steam3(const char* steam3);

// Renders as Steam2 string
char* sk_steam_id_render_steam2(const sk_steam_id_t* sid);

// Renders as Steam3 string
char* sk_steam_id_render_steam3(const sk_steam_id_t* sid);

#ifdef __cplusplus
}
#endif

#endif // STEAMKIT_TYPES_STEAM_ID_H
