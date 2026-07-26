#ifndef STEAMKIT_CDN_CLIENT_H
#define STEAMKIT_CDN_CLIENT_H

#include "steamkit/steam/steam_client.h"
#include "steamkit/cdn/cdn_server.h"
#include "steamkit/types/depot_manifest.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sk_cdn_client sk_cdn_client_t;
typedef struct sk_cdn_client_pool sk_cdn_client_pool_t;

sk_cdn_client_t* sk_cdn_client_create(sk_steam_client_t* steam_client);
void sk_cdn_client_destroy(sk_cdn_client_t* client);

sk_depot_manifest_t* sk_cdn_client_download_manifest(
    sk_cdn_client_t* client,
    uint32_t depot_id,
    uint64_t manifest_id,
    uint64_t manifest_request_code,
    sk_cdn_server_t* server,
    const uint8_t* depot_key,
    size_t key_len,
    const char* cdn_token);

int sk_cdn_client_download_depot_chunk(
    sk_cdn_client_t* client,
    uint32_t depot_id,
    const sk_depot_chunk_t* chunk,
    sk_cdn_server_t* server,
    uint8_t* buffer,
    size_t buffer_size,
    const uint8_t* depot_key,
    size_t key_len,
    const char* cdn_token);

sk_cdn_client_pool_t* sk_cdn_client_pool_create(void);
void sk_cdn_client_pool_destroy(sk_cdn_client_pool_t* pool);

void sk_cdn_client_pool_add_server(sk_cdn_client_pool_t* pool, const sk_cdn_server_t* server);
const sk_cdn_server_t* sk_cdn_client_pool_select_server(sk_cdn_client_pool_t* pool, bool prefer_https);
size_t sk_cdn_client_pool_count(const sk_cdn_client_pool_t* pool);
const sk_cdn_server_t** sk_cdn_client_pool_get_servers(const sk_cdn_client_pool_t* pool, size_t* out_count);

#ifdef __cplusplus
}
#endif

#endif // STEAMKIT_CDN_CLIENT_H
