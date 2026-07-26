#include "steamkit/utils/http_client.h"
#include <stdlib.h>
#include <string.h>

#ifdef SK_ENABLE_CURL

#include <curl/curl.h>

static size_t write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t total = size * nmemb;
    sk_http_response_t* response = (sk_http_response_t*)userp;
    char* new_body = (char*)realloc(response->body, response->body_len + total + 1);
    if (!new_body) return 0;
    response->body = new_body;
    memcpy(response->body + response->body_len, contents, total);
    response->body_len += total;
    response->body[response->body_len] = '\0';
    return total;
}

sk_http_client_t* sk_http_client_create(void) {
    sk_http_client_t* client = (sk_http_client_t*)calloc(1, sizeof(*client));
    if (!client) return NULL;
    client->curl = curl_easy_init();
    if (!client->curl) {
        free(client);
        return NULL;
    }
    curl_easy_setopt(client->curl, CURLOPT_USERAGENT, "Valve/Steam HTTP Client 1.0");
    curl_easy_setopt(client->curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(client->curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(client->curl, CURLOPT_SSL_VERIFYHOST, 2L);
    return client;
}

void sk_http_client_destroy(sk_http_client_t* client) {
    if (!client) return;
    if (client->curl) curl_easy_cleanup(client->curl);
    free(client);
}

void sk_http_client_set_timeout(sk_http_client_t* client, long connect_timeout_ms, long max_time_ms) {
    if (!client || !client->curl) return;
    curl_easy_setopt(client->curl, CURLOPT_CONNECTTIMEOUT_MS, connect_timeout_ms);
    curl_easy_setopt(client->curl, CURLOPT_TIMEOUT_MS, max_time_ms);
}

sk_http_response_t* sk_http_client_get(sk_http_client_t* client, const char* url, const char* const* headers) {
    if (!client || !client->curl || !url) return NULL;

    sk_http_response_t* response = (sk_http_response_t*)calloc(1, sizeof(*response));
    if (!response) return NULL;

    curl_easy_setopt(client->curl, CURLOPT_URL, url);
    curl_easy_setopt(client->curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(client->curl, CURLOPT_WRITEDATA, response);
    curl_easy_setopt(client->curl, CURLOPT_ERRORBUFFER, client->error_buffer);
    client->error_buffer[0] = '\0';

    if (headers && headers[0]) {
        struct curl_slist* list = NULL;
        for (size_t i = 0; headers[i]; ++i) {
            list = curl_slist_append(list, headers[i]);
        }
        curl_easy_setopt(client->curl, CURLOPT_HTTPHEADER, list);
        CURLcode res = curl_easy_perform(client->curl);
        curl_slist_free_all(list);
        (void)res;
    } else {
        curl_easy_setopt(client->curl, CURLOPT_HTTPHEADER, NULL);
        curl_easy_perform(client->curl);
    }

    curl_easy_getinfo(client->curl, CURLINFO_RESPONSE_CODE, &response->status_code);
    if (response->status_code == 0) {
        response->status_code = 0;
    }
    return response;
}

void sk_http_response_destroy(sk_http_response_t* response) {
    if (!response) return;
    free(response->body);
    free(response);
}

#endif
