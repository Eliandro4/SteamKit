#ifndef STEAMKIT_STEAM_HANDLERS_STEAM_SCREENSHOTS_H
#define STEAMKIT_STEAM_HANDLERS_STEAM_SCREENSHOTS_H

#include <stdint.h>
#include <stdbool.h>
#include "steamkit/steam/handlers/client_msg_handler.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sk_steam_screenshots sk_steam_screenshots_t;

sk_steam_screenshots_t* sk_steam_screenshots_create(void);
void sk_steam_screenshots_destroy(sk_steam_screenshots_t* screenshots);
void sk_steam_screenshots_take_screenshot(sk_steam_screenshots_t* screenshots);

#ifdef __cplusplus
}
#endif

#endif // STEAMKIT_STEAM_HANDLERS_STEAM_SCREENSHOTS_H
