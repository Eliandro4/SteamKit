#ifndef STEAMKIT_TYPES_GAME_ID_H
#define STEAMKIT_TYPES_GAME_ID_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// GameType mirrors C# GameID.GameType
typedef enum sk_game_type {
    SK_GAME_TYPE_APP = 0,
    SK_GAME_TYPE_GAME_MOD = 1,
    SK_GAME_TYPE_SHORTCUT = 2,
    SK_GAME_TYPE_P2P = 3
} sk_game_type_t;

// Forward declaration of Opaque GameID type
typedef struct sk_game_id sk_game_id_t;

// Value representation (stack-allocatable)
typedef struct {
    uint64_t id;
} sk_game_id_value_t;

// Creates a new GameID from a 64-bit integer
sk_game_id_t* sk_game_id_create(uint64_t id);

// Creates a new GameID from an app ID
sk_game_id_t* sk_game_id_create_from_app(uint32_t app_id);

// Creates a new GameID for a mod
sk_game_id_t* sk_game_id_create_mod(uint32_t app_id, const char* mod_path);

// Creates a new GameID for a shortcut
sk_game_id_t* sk_game_id_create_shortcut(const char* exe_path, const char* app_name);

// Destroys a GameID
void sk_game_id_destroy(sk_game_id_t* gid);

// Sets the GameID from a 64-bit integer
void sk_game_id_set(sk_game_id_t* gid, uint64_t id);

// Gets the 64-bit integer representation
uint64_t sk_game_id_to_uint64(const sk_game_id_t* gid);

// Gets the app ID
uint32_t sk_game_id_app_id(const sk_game_id_t* gid);

// Sets the app ID
void sk_game_id_set_app_id(sk_game_id_t* gid, uint32_t app_id);

// Gets the app type
sk_game_type_t sk_game_id_app_type(const sk_game_id_t* gid);

// Sets the app type
void sk_game_id_set_app_type(sk_game_id_t* gid, sk_game_type_t type);

// Gets the mod ID
uint32_t sk_game_id_mod_id(const sk_game_id_t* gid);

// Sets the mod ID
void sk_game_id_set_mod_id(sk_game_id_t* gid, uint32_t mod_id);

// Checks if this is a mod
bool sk_game_id_is_mod(const sk_game_id_t* gid);

// Checks if this is a shortcut
bool sk_game_id_is_shortcut(const sk_game_id_t* gid);

// Checks if this is a P2P file
bool sk_game_id_is_p2p(const sk_game_id_t* gid);

// Checks if this is a Steam app
bool sk_game_id_is_steam_app(const sk_game_id_t* gid);

// Equality check
bool sk_game_id_equals(const sk_game_id_t* a, const sk_game_id_t* b);

// String representation (caller must free)
char* sk_game_id_to_string(const sk_game_id_t* gid);

#ifdef __cplusplus
}
#endif

#endif // STEAMKIT_TYPES_GAME_ID_H
