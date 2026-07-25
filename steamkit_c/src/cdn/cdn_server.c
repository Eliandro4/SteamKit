#include "steamkit/cdn/cdn_server.h"
#include <stdlib.h>
#include <string.h>

static char* sk_strdup(const char* s) {
    if (!s) return NULL;
    size_t len = strlen(s) + 1;
    char* dup = (char*)malloc(len);
    if (dup) memcpy(dup, s, len);
    return dup;
}

sk_cdn_server_t* sk_cdn_server_create(const char* type, const char* host) {
    sk_cdn_server_t* server = (sk_cdn_server_t*)calloc(1, sizeof(sk_cdn_server_t));
    if (server) {
        server->type = sk_strdup(type);
        server->host = sk_strdup(host);
    }
    return server;
}

void sk_cdn_server_destroy(sk_cdn_server_t* server) {
    if (server) {
        free(server->type);
        free(server->host);
        free(server);
    }
}
