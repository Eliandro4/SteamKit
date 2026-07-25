#ifndef STEAMKIT_CDN_SERVER_H
#define STEAMKIT_CDN_SERVER_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sk_cdn_server {
    char* type;
    char* host;
} sk_cdn_server_t;

sk_cdn_server_t* sk_cdn_server_create(const char* type, const char* host);
void sk_cdn_server_destroy(sk_cdn_server_t* server);

#ifdef __cplusplus
}
#endif

#endif // STEAMKIT_CDN_SERVER_H
