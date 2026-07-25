#include <stdio.h>
#include <string.h>
#include <steamkit/steamkit.h>

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    printf("SteamKit C - Login Sample\n");

    // Create configuration
    sk_steam_configuration_t* config = sk_steam_configuration_create_default();
    if (!config) {
        fprintf(stderr, "Failed to create configuration\n");
        return 1;
    }

    // Create client
    sk_steam_client_t* client = sk_steam_client_create_with_config(config);
    if (!client) {
        fprintf(stderr, "Failed to create client\n");
        sk_steam_configuration_destroy(config);
        return 1;
    }

    // Connect
    sk_steam_client_connect(client);
    printf("Connecting...\n");

    // Note: Full async operation requires event loop integration
    // This is a structural sample showing the API

    // Cleanup
    sk_steam_client_disconnect(client, true);
    sk_steam_client_destroy(client);
    sk_steam_configuration_destroy(config);

    printf("Done.\n");
    return 0;
}
