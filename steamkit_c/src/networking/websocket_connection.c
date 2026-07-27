#define _GNU_SOURCE
#include "steamkit/networking/websocket_connection.h"
#include "steamkit/networking/connection.h"
#include "steamkit/base/emsg.h"
#include "steamkit/base/msg_hdr.h"
#include <stdlib.h>
#include <string.h>

#ifdef SK_ENABLE_CURL
#include <curl/curl.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#endif

typedef struct sk_websocket_connection {
    sk_connection_t base;
    sk_websocket_context_t context;
#ifdef SK_ENABLE_CURL
    CURL* curl;
    bool connected;
    bool running;
    pthread_t thread;
    uint8_t* recv_buf;
    size_t recv_len;
    size_t recv_cap;
    pthread_mutex_t buf_mutex;
    pthread_cond_t buf_cond;
    char error_buf[CURL_ERROR_SIZE];
#endif
} sk_websocket_connection_t;

#ifdef SK_ENABLE_CURL

#define WS_TCP_MAGIC 0x31305456U

static void ws_process_pending_messages(sk_websocket_connection_t* ws) {
    if (!ws || !ws->base.net_msg_callback) return;

    while (ws->recv_len >= 24) {
        uint32_t msg_len;
        memcpy(&msg_len, ws->recv_buf, 4);
        uint32_t msg_magic;
        memcpy(&msg_magic, ws->recv_buf + 4, 4);
        if (msg_magic != WS_TCP_MAGIC) {
            break;
        }
        if (ws->recv_len < 8U + (size_t)msg_len) break;

        const uint8_t* payload = ws->recv_buf + 8;
        size_t payload_len = (size_t)msg_len;

        sk_emsg_t emsg = SK_EMSG_INVALID;
        if (payload_len >= 20) {
            sk_msg_hdr_t hdr;
            if (sk_msg_hdr_deserialize(&hdr, payload, payload_len)) {
                emsg = hdr.msg;
            }
        }

        ws->base.net_msg_callback(ws->base.user_data,
                                    payload,
                                    payload_len,
                                    emsg);

        memmove(ws->recv_buf,
                ws->recv_buf + 8 + (size_t)msg_len,
                ws->recv_len - 8U - (size_t)msg_len);
        ws->recv_len -= 8U + (size_t)msg_len;
    }
}

static void* ws_recv_thread(void* arg) {
    sk_websocket_connection_t* ws = (sk_websocket_connection_t*)arg;
    if (!ws || !ws->curl) return NULL;

    uint8_t tmp[4096];
    while (ws->running) {
        size_t nread = 0;
        const struct curl_ws_frame* metap = NULL;
        CURLcode rc = curl_ws_recv(ws->curl, tmp, sizeof(tmp), &nread, &metap);
        if (rc == CURLE_AGAIN) {
            usleep(1000);
            continue;
        }
        if (rc != CURLE_OK || nread == 0) {
            break;
        }

        pthread_mutex_lock(&ws->buf_mutex);
        if (ws->recv_len + nread > ws->recv_cap) {
            size_t new_cap = ws->recv_cap * 2 + nread;
            uint8_t* new_buf = (uint8_t*)realloc(ws->recv_buf, new_cap);
            if (!new_buf) {
                pthread_mutex_unlock(&ws->buf_mutex);
                break;
            }
            ws->recv_buf = new_buf;
            ws->recv_cap = new_cap;
        }
        memcpy(ws->recv_buf + ws->recv_len, tmp, nread);
        ws->recv_len += nread;
        ws_process_pending_messages(ws);
        pthread_cond_signal(&ws->buf_cond);
        pthread_mutex_unlock(&ws->buf_mutex);
    }

    ws->running = false;
    if (ws->base.disconnected_callback) {
        ws->base.disconnected_callback(ws->base.user_data, true);
    }
    return NULL;
}

static bool ws_do_connect(sk_websocket_connection_t* ws, const char* url, int timeout_ms) {
    if (!ws || !url) return false;

    ws->curl = curl_easy_init();
    if (!ws->curl) return false;

    curl_easy_setopt(ws->curl, CURLOPT_URL, url);
    curl_easy_setopt(ws->curl, CURLOPT_CONNECT_ONLY, 2L);
    curl_easy_setopt(ws->curl, CURLOPT_TIMEOUT_MS, (long)timeout_ms);
    curl_easy_setopt(ws->curl, CURLOPT_ERRORBUFFER, ws->error_buf);
    curl_easy_setopt(ws->curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(ws->curl, CURLOPT_SSL_VERIFYHOST, 2L);

    CURLcode rc = curl_easy_perform(ws->curl);
    if (rc != CURLE_OK) {
        curl_easy_cleanup(ws->curl);
        ws->curl = NULL;
        return false;
    }

    ws->recv_cap = 4096;
    ws->recv_buf = (uint8_t*)malloc(ws->recv_cap);
    if (!ws->recv_buf) {
        curl_easy_cleanup(ws->curl);
        ws->curl = NULL;
        return false;
    }

    pthread_mutex_init(&ws->buf_mutex, NULL);
    pthread_cond_init(&ws->buf_cond, NULL);

    ws->running = true;
    if (pthread_create(&ws->thread, NULL, ws_recv_thread, ws) != 0) {
        free(ws->recv_buf);
        ws->recv_buf = NULL;
        curl_easy_cleanup(ws->curl);
        ws->curl = NULL;
        return false;
    }

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
    sk_websocket_connection_disconnect(ws, true);
#ifdef SK_ENABLE_CURL
    pthread_mutex_destroy(&ws->buf_mutex);
    pthread_cond_destroy(&ws->buf_cond);
#endif
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
#ifdef SK_ENABLE_CURL
    if (ws->curl) {
        ws->running = false;
        curl_easy_cleanup(ws->curl);
        ws->curl = NULL;
        if (ws->thread) {
            pthread_join(ws->thread, NULL);
            ws->thread = 0;
        }
        free(ws->recv_buf);
        ws->recv_buf = NULL;
        ws->recv_len = 0;
        ws->recv_cap = 0;
    }
#endif
    ws->connected = false;
    ws->base.is_connected = false;
    if (ws->base.disconnected_callback) {
        ws->base.disconnected_callback(ws->base.user_data, user_initiated);
    }
}

void sk_websocket_connection_send(sk_websocket_connection_t* ws, const uint8_t* data, size_t len) {
    if (!ws || !data || len == 0) return;
#ifdef SK_ENABLE_CURL
    if (!ws->curl || !ws->connected) return;
    size_t sent = 0;
    curl_ws_send(ws->curl, data, len, &sent, 0, CURLWS_BINARY);
#else
    (void)data;
    (void)len;
#endif
}

ssize_t sk_websocket_connection_recv(sk_websocket_connection_t* ws, uint8_t* buf, size_t buf_len, int timeout_ms) {
    if (!ws || !buf || buf_len == 0) return -1;
#ifdef SK_ENABLE_CURL
    if (!ws->curl || !ws->connected) return -1;

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += timeout_ms / 1000;
    ts.tv_nsec += (timeout_ms % 1000) * 1000000;
    if (ts.tv_nsec >= 1000000000) {
        ts.tv_sec += 1;
        ts.tv_nsec -= 1000000000;
    }

    pthread_mutex_lock(&ws->buf_mutex);
    while (ws->running && ws->recv_len == 0) {
        if (timeout_ms < 0) {
            pthread_cond_wait(&ws->buf_cond, &ws->buf_mutex);
        } else {
            if (pthread_cond_timedwait(&ws->buf_cond, &ws->buf_mutex, &ts) != 0) {
                pthread_mutex_unlock(&ws->buf_mutex);
                return 0;
            }
        }
    }
    if (!ws->running && ws->recv_len == 0) {
        pthread_mutex_unlock(&ws->buf_mutex);
        return 0;
    }

    size_t to_copy = ws->recv_len < buf_len ? ws->recv_len : buf_len;
    memcpy(buf, ws->recv_buf, to_copy);
    if (to_copy < ws->recv_len) {
        memmove(ws->recv_buf, ws->recv_buf + to_copy, ws->recv_len - to_copy);
    }
    ws->recv_len -= to_copy;
    pthread_mutex_unlock(&ws->buf_mutex);
    return (ssize_t)to_copy;
#else
    (void)buf;
    (void)buf_len;
    (void)timeout_ms;
    return -1;
#endif
}

