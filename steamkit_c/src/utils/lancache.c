#define _GNU_SOURCE
#include "steamkit/utils/lancache.h"
#include "steamkit/utils/http_client.h"
#include "steamkit/utils/debug_log.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define SK_LANCACHE_DETECT_TIMEOUT_MS 5000

static bool sk_lancache_check_host(const char* host, uint16_t port) {
    if (!host) return false;

    char url[256];
    snprintf(url, sizeof(url), "http://%s:%u/", host, port);

    sk_http_client_t* client = sk_http_client_create();
    if (!client) return false;

    sk_http_client_set_timeout(client, 2000, 3000);
    sk_http_response_t* resp = sk_http_client_get(client, url, NULL);
    sk_http_client_destroy(client);

    if (!resp) return false;

    bool is_lancache = (resp->status_code == 200 || resp->status_code == 404);
    sk_http_response_destroy(resp);
    return is_lancache;
}

bool sk_lancache_detect(const char* host, uint16_t port) {
    if (!host) return false;
    bool result = sk_lancache_check_host(host, port);
    sk_debug_log_info("Lancache", "Detection for %s:%u -> %s", host, port, result ? "LANCACHE" : "NOT_LANCACHE");
    return result;
}

sk_lancache_info_t* sk_lancache_probe(const char* host, uint16_t port) {
    if (!host) return NULL;

    sk_lancache_info_t* info = (sk_lancache_info_t*)calloc(1, sizeof(sk_lancache_info_t));
    if (!info) return NULL;

    info->host = strdup(host);
    info->port = port;
    info->is_lancache = sk_lancache_detect(host, port);

    if (info->is_lancache) {
        sk_debug_log_info("Lancache", "Detected Lancache at %s:%u", host, port);
    }

    return info;
}

void sk_lancache_info_destroy(sk_lancache_info_t* info) {
    if (!info) return;
    free(info->host);
    free(info);
}