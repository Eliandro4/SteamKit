#ifndef STEAMKIT_STEAM_STEAM_CONFIGURATION_H
#define STEAMKIT_STEAM_STEAM_CONFIGURATION_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum sk_client_persona_state_flag {
    SK_PERSONA_STATE_FLAG_NONE = 0,
    SK_PERSONA_STATE_FLAG_HAS_RICH_PRESENCE = 1,
    SK_PERSONA_STATE_FLAG_IN_JOINABLE_GAME = 2,
    SK_PERSONA_STATE_FLAG_GOLDEN = 4,
    SK_PERSONA_STATE_FLAG_REMOTE_PLAY_TOGETHER = 8,
    SK_PERSONA_STATE_FLAG_CLIENT_TYPE_WEB = 256,
    SK_PERSONA_STATE_FLAG_CLIENT_TYPE_MOBILE = 512,
    SK_PERSONA_STATE_FLAG_CLIENT_TYPE_TENFOOT = 1024,
    SK_PERSONA_STATE_FLAG_CLIENT_TYPE_VR = 2048,
} sk_client_persona_state_flag_t;

// Configuration for SteamClient - mirrors C# SteamConfiguration
typedef struct sk_steam_configuration sk_steam_configuration_t;

// Creates a new configuration with default settings
sk_steam_configuration_t* sk_steam_configuration_create_default(void);

// Creates a new configuration with options
sk_steam_configuration_t* sk_steam_configuration_create(void);

// Destroys a configuration
void sk_steam_configuration_destroy(sk_steam_configuration_t* config);

// Gets whether directory fetch is allowed
bool sk_steam_configuration_allow_directory_fetch(const sk_steam_configuration_t* config);

// Gets the Cell ID
uint32_t sk_steam_configuration_cell_id(const sk_steam_configuration_t* config);

// Gets default persona state flags
sk_client_persona_state_flag_t sk_steam_configuration_default_persona_state_flags(const sk_steam_configuration_t* config);

// Gets the connection timeout
int sk_steam_configuration_connection_timeout_ms(const sk_steam_configuration_t* config);

#ifdef __cplusplus
}
#endif

#endif // STEAMKIT_STEAM_STEAM_CONFIGURATION_H
