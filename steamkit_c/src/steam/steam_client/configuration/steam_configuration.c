#include "steamkit/steam/steam_client/configuration/steam_configuration.h"
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

struct sk_steam_configuration {
    bool allow_directory_fetch;
    uint32_t cell_id;
    sk_client_persona_state_flag_t default_persona_state_flags;
    int connection_timeout_ms;
};

sk_steam_configuration_t* sk_steam_configuration_create_default(void) {
    sk_steam_configuration_t* config = (sk_steam_configuration_t*)malloc(sizeof(sk_steam_configuration_t));
    if (config) {
        config->allow_directory_fetch = true;
        config->cell_id = 0;
        config->default_persona_state_flags = SK_PERSONA_STATE_FLAG_HAS_RICH_PRESENCE;
        config->connection_timeout_ms = 5000;
    }
    return config;
}

sk_steam_configuration_t* sk_steam_configuration_create(void) {
    return sk_steam_configuration_create_default();
}

void sk_steam_configuration_destroy(sk_steam_configuration_t* config) {
    free(config);
}

bool sk_steam_configuration_allow_directory_fetch(const sk_steam_configuration_t* config) {
    return config ? config->allow_directory_fetch : true;
}

uint32_t sk_steam_configuration_cell_id(const sk_steam_configuration_t* config) {
    return config ? config->cell_id : 0;
}

sk_client_persona_state_flag_t sk_steam_configuration_default_persona_state_flags(const sk_steam_configuration_t* config) {
    return config ? config->default_persona_state_flags : SK_PERSONA_STATE_FLAG_HAS_RICH_PRESENCE;
}

int sk_steam_configuration_connection_timeout_ms(const sk_steam_configuration_t* config) {
    return config ? config->connection_timeout_ms : 5000;
}
