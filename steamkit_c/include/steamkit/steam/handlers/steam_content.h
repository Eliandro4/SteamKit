#ifndef STEAMKIT_STEAM_HANDLERS_STEAM_CONTENT_H
#define STEAMKIT_STEAM_HANDLERS_STEAM_CONTENT_H

#include <stdint.h>
#include <stdbool.h>
#include "steamkit/steam/handlers/client_msg_handler.h"
#include "steamkit/steam/callbacks.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sk_steam_content sk_steam_content_t;

sk_steam_content_t* sk_steam_content_create(void);
void sk_steam_content_destroy(sk_steam_content_t* content);

sk_cdn_server_list_callback_t* sk_steam_content_get_servers_for_steam_pipe(sk_steam_content_t* content);

sk_manifest_request_code_callback_t* sk_steam_content_get_manifest_request_code(
    sk_steam_content_t* content,
    uint32_t depot_id,
    uint32_t app_id,
    uint64_t manifest_id,
    const char* branch);

sk_cdn_auth_token_callback_t* sk_steam_content_get_cdn_auth_token(
    sk_steam_content_t* content,
    uint32_t app_id,
    uint32_t depot_id,
    const char* host);

#ifdef __cplusplus
}
#endif

#endif // STEAMKIT_STEAM_HANDLERS_STEAM_CONTENT_H
