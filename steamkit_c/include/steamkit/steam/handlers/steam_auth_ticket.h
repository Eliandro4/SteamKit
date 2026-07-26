#ifndef STEAMKIT_STEAM_HANDLERS_STEAM_AUTH_TICKET_H
#define STEAMKIT_STEAM_HANDLERS_STEAM_AUTH_TICKET_H

#include <stdint.h>
#include <stdbool.h>
#include "steamkit/steam/handlers/client_msg_handler.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sk_steam_auth_ticket sk_steam_auth_ticket_t;

sk_steam_auth_ticket_t* sk_steam_auth_ticket_create(void);
void sk_steam_auth_ticket_destroy(sk_steam_auth_ticket_t* auth_ticket);
void sk_steam_auth_ticket_begin_session(sk_steam_auth_ticket_t* auth_ticket, uint32_t app_id);

#ifdef __cplusplus
}
#endif

#endif // STEAMKIT_STEAM_HANDLERS_STEAM_AUTH_TICKET_H
