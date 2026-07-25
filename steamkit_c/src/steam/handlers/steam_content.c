#include "steamkit/steam/handlers/steam_content.h"
#include "steamkit/steam/handlers/client_msg_handler.h"
#include <stdlib.h>

static void sk_steam_content_handle_msg(struct sk_client_msg_handler* handler, const sk_packet_msg_t* packet_msg) {
    (void)handler;
    (void)packet_msg;
}

typedef struct sk_steam_content {
    struct sk_client_msg_handler base;
} sk_steam_content_t;

sk_steam_content_t* sk_steam_content_create(void) {
    sk_steam_content_t* content = (sk_steam_content_t*)calloc(1, sizeof(sk_steam_content_t));
    if (content) {
        content->base.handle_msg = sk_steam_content_handle_msg;
    }
    return content;
}

void sk_steam_content_destroy(sk_steam_content_t* content) {
    free(content);
}

sk_cdn_server_list_t* sk_steam_content_get_servers_for_steam_pipe(sk_steam_content_t* content) {
    (void)content;
    // STUB
    return NULL;
}

sk_manifest_request_code_callback_t* sk_steam_content_get_manifest_request_code(
    sk_steam_content_t* content,
    uint32_t depot_id,
    uint32_t app_id,
    uint64_t manifest_id,
    const char* branch) {
    (void)content;
    (void)depot_id;
    (void)app_id;
    (void)manifest_id;
    (void)branch;
    // STUB
    return NULL;
}

sk_cdn_auth_token_callback_t* sk_steam_content_get_cdn_auth_token(
    sk_steam_content_t* content,
    uint32_t app_id,
    uint32_t depot_id,
    const char* host) {
    (void)content;
    (void)app_id;
    (void)depot_id;
    (void)host;
    // STUB
    return NULL;
}
