#include "steamkit/networking/websocket_connection.h"
#include "steamkit/networking/connection.h"
#include <stdlib.h>
#include <string.h>

#ifdef SK_ENABLE_CURL
#include <curl/curl.h>
#include <pthread.h>
#endif

typedef struct sk_websocket_connection {
    sk_connection_t base;
    sk_websocket_context_t context;
    void* http_client;
    bool connected;
} sk_websocket_connection_t;

#ifdef SK_ENABLE_CURL

typedef struct {
    sk_websocket_connection_t* ws;
    CURL* curl;
    bool running;
    pthread_t thread;
    uint8_t* buf;
    size_t buf_len;
    size_t buf_cap;
    char error_buf[CURL_ERROR_SIZE];
} ws_internal_t;

static size_t ws_write_cb(char* ptr, size_t size, size_t nmemb, void* userdata) {
    ws_internal_t* internal = (ws_internal_t*)userdata;
    size_t total = size * nmemb;
    if (!internal || !internal->buf) return 0;
    if (internal->buf_len + total > internal->buf_cap) {
        size_t new_cap = internal->buf_cap * 2 + total;
        uint8_t* new_buf = (uint8_t*)realloc(internal->buf, new_cap);
        if (!new_buf) return 0;
        internal->buf = new_buf;
        internal->buf_cap = new_cap;
    }
    memcpy(internal->buf + internal->buf_len, ptr, total);
    internal->buf_len += total;
    return total;
}

static void* ws_recv_thread(void* arg) {
    ws_internal_t* internal = (ws_internal_t*)arg;
    if (!internal) return NULL;

    sk_websocket_connection_t* ws = internal->ws;
    CURLcode res = curl_easy_perform(internal->curl);

    internal->running = false;

    if (res != CURLE_OK && internal->buf_len == 0) {
        if (ws->base.disconnected_callback) {
            ws->base.disconnected_callback(ws->base.user_data, true);
        }
        return NULL;
    }

    if (internal->buf_len > 0 && ws->base.net_msg_callback) {
        ws->base.net_msg_callback(ws->base.user_data,
                                    internal->buf,
                                    internal->buf_len,
                                    SK_EMSG_INVALID);
    }

    if (ws->base.disconnected_callback) {
        ws->base.disconnected_callback(ws->base.user_data, true);
    }

    return NULL;
}

static bool ws_do_connect(sk_websocket_connection_t* ws, const char* url, int timeout_ms) {
    ws_internal_t* internal = (ws_internal_t*)calloc(1, sizeof(ws_internal_t));
    if (!internal) return false;

    internal->ws = ws;
    internal->curl = curl_easy_init();
    if (!internal->curl) {
        free(internal);
        return false;
    }

    internal->buf_cap = 4096;
    internal->buf = (uint8_t*)malloc(internal->buf_cap);
    if (!internal->buf) {
        curl_easy_cleanup(internal->curl);
        free(internal);
        return false;
    }

    curl_easy_setopt(internal->curl, CURLOPT_URL, url);
    curl_easy_setopt(internal->curl, CURLOPT_WRITEFUNCTION, ws_write_cb);
    curl_easy_setopt(internal->curl, CURLOPT_WRITEDATA, internal);
    curl_easy_setopt(internal->curl, CURLOPT_CONNECTTIMEOUT_MS, timeout_ms);
    curl_easy_setopt(internal->curl, CURLOPT_ERRORBUFFER, internal->error_buf);
    curl_easy_setopt(internal->curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(internal->curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(internal->curl, CURLOPT_SSL_VERIFYHOST, 2L);

    internal->running = true;
    if (pthread_create(&internal->thread, NULL, ws_recv_thread, internal) != 0) {
        curl_easy_cleanup(internal->curl);
        free(internal->buf);
        free(internal);
        return false;
    }

    ws->http_client = internal;
    ws->connected = true;
    ws->base.is_connected = true;

    if (ws->base.connected_callback) {
        ws->base.connected_callback(ws->base.user_data);
    }

    return true;
}

#endif

static char* sk_strdup(const char* s) {
    if (!s) return NULL;
    size_t len = strlen(s) + 1;
    char* dup = (char*)malloc(len);
    if (dup) memcpy(dup, s, len);
    return dup;
}

sk_websocket_connection_t* sk_websocket_connection_create(const sk_websocket_context_t* context) {
    sk_websocket_connection_t* ws = (sk_websocket_connection_t*)calloc(1, sizeof(sk_websocket_connection_t));
    if (!ws) return NULL;
    if (context) {
        ws->base.protocol = SK_PROTOCOL_TYPE_WEBSOCKET;
        ws->base.impl = ws;
        ws->context.url = sk_strdup(context->url);
        ws->context.user_agent = sk_strdup(context->user_agent);
        ws->context.subprotocol_count = context->subprotocol_count;
        if (context->subprotocol_count > 0 && context->subprotocols) {
            ws->context.subprotocols = (const char**)malloc(context->subprotocol_count * sizeof(const char*));
            if (ws->context.subprotocols) {
                for (size_t i = 0; i < context->subprotocol_count; ++i) {
                    ws->context.subprotocols[i] = sk_strdup(context->subprotocols[i]);
                }
            }
        } else {
            ws->context.subprotocols = NULL;
        }
    }
    return ws;
}

void sk_websocket_connection_destroy(sk_websocket_connection_t* ws) {
    if (!ws) return;
    if (ws->http_client) {
#ifdef SK_ENABLE_CURL
        ws_internal_t* internal = (ws_internal_t*)ws->http_client;
        internal->running = false;
        curl_easy_cleanup(internal->curl);
        pthread_join(internal->thread, NULL);
        free(internal->buf);
        free(internal);
#endif
        ws->http_client = NULL;
    }
    free((void*)ws->context.url);
    if (ws->context.subprotocols) {
        for (size_t i = 0; i < ws->context.subprotocol_count; ++i) {
            free((void*)ws->context.subprotocols[i]);
        }
        free((void*)ws->context.subprotocols);
    }
    free((void*)ws->context.user_agent);
    free(ws);
}

bool sk_websocket_connection_connect(sk_websocket_connection_t* ws, const char* url, int timeout_ms) {
    if (!ws || !url) return false;
#ifdef SK_ENABLE_CURL
    return ws_do_connect(ws, url, timeout_ms);
#else
    (void)url;
    (void)timeout_ms;
    return false;
#endif
}

void sk_websocket_connection_disconnect(sk_websocket_connection_t* ws, bool user_initiated) {
    if (!ws) return;
    if (ws->http_client) {
#ifdef SK_ENABLE_CURL
        ws_internal_t* internal = (ws_internal_t*)ws->http_client;
        internal->running = false;
        curl_easy_cleanup(internal->curl);
        pthread_join(internal->thread, NULL);
        free(internal->buf);
        free(internal);
#endif
        ws->http_client = NULL;
    }
    ws->connected = false;
    if (ws->base.disconnected_callback) {
        ws->base.disconnected_callback(ws->base.user_data, user_initiated);
    }
}
