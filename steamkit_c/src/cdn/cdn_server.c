#include "steamkit/cdn/cdn_server.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

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
        server->port = 80;
        server->cell_id = 0;
        server->load = 0;
        server->weighted_load = 0;
        server->num_entries = 0;
        server->use_as_proxy = false;
        server->proxy_request_path_template = NULL;
        server->https_support = NULL;
        server->allowed_app_ids = NULL;
        server->num_allowed_app_ids = 0;
    }
    return server;
}

void sk_cdn_server_destroy(sk_cdn_server_t* server) {
    if (server) {
        free(server->type);
        free(server->host);
        free(server->vhost);
        free(server->proxy_request_path_template);
        free(server->https_support);
        free(server->allowed_app_ids);
        free(server);
    }
}

sk_cdn_server_t* sk_cdn_server_clone(const sk_cdn_server_t* server) {
    if (!server) return NULL;
    sk_cdn_server_t* clone = (sk_cdn_server_t*)calloc(1, sizeof(sk_cdn_server_t));
    if (!clone) return NULL;
    clone->type = sk_strdup(server->type);
    clone->host = sk_strdup(server->host);
    clone->vhost = sk_strdup(server->vhost);
    clone->port = server->port;
    clone->cell_id = server->cell_id;
    clone->load = server->load;
    clone->weighted_load = server->weighted_load;
    clone->num_entries = server->num_entries;
    clone->use_as_proxy = server->use_as_proxy;
    clone->proxy_request_path_template = sk_strdup(server->proxy_request_path_template);
    clone->https_support = sk_strdup(server->https_support);
    if (server->num_allowed_app_ids > 0 && server->allowed_app_ids) {
        clone->allowed_app_ids = (uint32_t*)malloc(server->num_allowed_app_ids * sizeof(uint32_t));
        if (clone->allowed_app_ids) {
            memcpy(clone->allowed_app_ids, server->allowed_app_ids, server->num_allowed_app_ids * sizeof(uint32_t));
            clone->num_allowed_app_ids = server->num_allowed_app_ids;
        }
    }
    return clone;
}
