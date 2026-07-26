#ifndef STEAMKIT_UTILS_HTTP_CLIENT_H
#define STEAMKIT_UTILS_HTTP_CLIENT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sk_http_response {
    long status_code;
    char* body;
    size_t body_len;
} sk_http_response_t;

#ifdef SK_ENABLE_CURL

#include <curl/curl.h>

typedef struct sk_http_client {
    CURL* curl;
    char error_buffer[CURL_ERROR_SIZE];
} sk_http_client_t;

sk_http_client_t* sk_http_client_create(void);
void sk_http_client_destroy(sk_http_client_t* client);
void sk_http_client_set_timeout(sk_http_client_t* client, long connect_timeout_ms, long max_time_ms);
sk_http_response_t* sk_http_client_get(sk_http_client_t* client, const char* url, const char* const* headers);
void sk_http_response_destroy(sk_http_response_t* response);

#else

typedef struct sk_http_client {
    void* dummy;
} sk_http_client_t;

static inline sk_http_client_t* sk_http_client_create(void) { return NULL; }
static inline void sk_http_client_destroy(sk_http_client_t* client) { (void)client; }
static inline void sk_http_client_set_timeout(sk_http_client_t* client, long connect_timeout_ms, long max_time_ms) { (void)client; (void)connect_timeout_ms; (void)max_time_ms; }
static inline sk_http_response_t* sk_http_client_get(sk_http_client_t* client, const char* url, const char* const* headers) { (void)client; (void)url; (void)headers; return NULL; }
static inline void sk_http_response_destroy(sk_http_response_t* response) { (void)response; }

#endif

#ifdef __cplusplus
}
#endif

#endif // STEAMKIT_UTILS_HTTP_CLIENT_H
