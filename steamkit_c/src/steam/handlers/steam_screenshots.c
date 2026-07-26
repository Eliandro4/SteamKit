#include "steamkit/steam/handlers/steam_screenshots.h"
#include "steamkit/steam/handlers/client_msg_handler.h"
#include <stdlib.h>

static void sk_steam_ss_handle_msg(struct sk_client_msg_handler* handler, const sk_packet_msg_t* packet_msg) {
    (void)handler;
    (void)packet_msg;
}

typedef struct sk_steam_screenshots {
    struct sk_client_msg_handler base;
} sk_steam_screenshots_t;

sk_steam_screenshots_t* sk_steam_screenshots_create(void) {
    sk_steam_screenshots_t* screenshots = (sk_steam_screenshots_t*)calloc(1, sizeof(sk_steam_screenshots_t));
    if (screenshots) {
        screenshots->base.handle_msg = sk_steam_ss_handle_msg;
        screenshots->base.handler_type = SK_HANDLER_STEAM_SCREENSHOTS;
    }
    return screenshots;
}

void sk_steam_screenshots_destroy(sk_steam_screenshots_t* screenshots) {
    free(screenshots);
}
