#include <stdio.h>
#include <string.h>
#include <steamkit/steamkit.h>

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    printf("SteamKit C - Friends Sample\n");

    sk_steam_configuration_t* config = sk_steam_configuration_create_default();
    sk_steam_client_t* client = sk_steam_client_create_with_config(config);
    sk_steam_user_t* user = sk_steam_user_create();
    sk_steam_friends_t* friends = sk_steam_friends_create();

    // Use the API...

    sk_steam_friends_destroy(friends);
    sk_steam_user_destroy(user);
    sk_steam_client_destroy(client);
    sk_steam_configuration_destroy(config);

    printf("Done.\n");
    return 0;
}
