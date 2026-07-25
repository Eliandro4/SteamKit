#include "steamkit/steam/handlers/steam_workshop.h"
#include "steamkit/steam/handlers/client_msg_handler.h"
#include <stdlib.h>

static void sk_steam_workshop_handle_msg(struct sk_client_msg_handler* handler, const sk_packet_msg_t* packet_msg) {
    (void)handler;
    (void)packet_msg;
}

typedef struct sk_steam_workshop {
    struct sk_client_msg_handler base;
} sk_steam_workshop_t;

sk_steam_workshop_t* sk_steam_workshop_create(void) {
    sk_steam_workshop_t* workshop = (sk_steam_workshop_t*)calloc(1, sizeof(sk_steam_workshop_t));
    if (workshop) {
        workshop->base.handle_msg = sk_steam_workshop_handle_msg;
    }
    return workshop;
}

void sk_steam_workshop_destroy(sk_steam_workshop_t* workshop) {
    free(workshop);
}
