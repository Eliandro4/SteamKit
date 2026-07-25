#ifndef STEAMKIT_STEAM_MSG_CLOUD_H
#define STEAMKIT_STEAM_MSG_CLOUD_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sk_ccloud_user_file {
    char* filename;
    // other fields omitted for brevity
} sk_ccloud_user_file_t;

typedef struct sk_ccloud_enumerate_user_files_request {
    uint32_t appid;
    bool extended_details;
    uint32_t count;
} sk_ccloud_enumerate_user_files_request_t;

typedef struct sk_ccloud_enumerate_user_files_response {
    sk_ccloud_user_file_t* files;
    uint32_t num_files;
} sk_ccloud_enumerate_user_files_response_t;

typedef struct sk_ccloud_client_file_download_request {
    uint32_t appid;
    char* filename;
} sk_ccloud_client_file_download_request_t;

typedef struct sk_ccloud_request_header {
    char* name;
    char* value;
} sk_ccloud_request_header_t;

typedef struct sk_ccloud_client_file_download_response {
    bool use_https;
    char* url_host;
    char* url_path;
    sk_ccloud_request_header_t* request_headers;
    uint32_t num_request_headers;
} sk_ccloud_client_file_download_response_t;

#ifdef __cplusplus
}
#endif

#endif // STEAMKIT_STEAM_MSG_CLOUD_H
