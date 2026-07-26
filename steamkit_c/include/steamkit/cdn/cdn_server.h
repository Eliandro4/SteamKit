#ifndef STEAMKIT_CDN_SERVER_H
#define STEAMKIT_CDN_SERVER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sk_cdn_server {
    char* type;
    char* host;
    char* vhost;
    int port;
    int cell_id;
    int load;
    float weighted_load;
    int num_entries;
    bool use_as_proxy;
    char* proxy_request_path_template;
    char* https_support;
    uint32_t* allowed_app_ids;
    uint32_t num_allowed_app_ids;
} sk_cdn_server_t;

sk_cdn_server_t* sk_cdn_server_create(const char* type, const char* host);
void sk_cdn_server_destroy(sk_cdn_server_t* server);
sk_cdn_server_t* sk_cdn_server_clone(const sk_cdn_server_t* server);

#ifdef __cplusplus
}
#endif

#endif // STEAMKIT_CDN_SERVER_H
