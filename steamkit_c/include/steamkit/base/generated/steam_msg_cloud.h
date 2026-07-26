#ifndef STEAMKIT_STEAM_MSG_CLOUD_H
#define STEAMKIT_STEAM_MSG_CLOUD_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sk_ccloud_user_file {
    char* filename;
    uint64_t ugc_id;
    uint64_t timestamp;
    uint32_t file_size;
    char* url;
    uint64_t steamid_creator;
    uint32_t appid;
} sk_ccloud_user_file_t;

typedef struct sk_ccloud_enumerate_user_files_request {
    uint32_t appid;
    bool extended_details;
    uint32_t count;
    uint32_t start_index;
} sk_ccloud_enumerate_user_files_request_t;

typedef struct sk_ccloud_enumerate_user_files_response {
    sk_ccloud_user_file_t* files;
    uint32_t num_files;
    uint32_t total_files;
} sk_ccloud_enumerate_user_files_response_t;

typedef struct sk_ccloud_client_file_download_request {
    uint32_t appid;
    char* filename;
    uint32_t realm;
    bool force_proxy;
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
    uint32_t file_size;
    uint32_t raw_file_size;
    bool encrypted;
    uint64_t timestamp;
    bool is_explicit_delete;
} sk_ccloud_client_file_download_response_t;

#ifdef __cplusplus
}
#endif

#endif // STEAMKIT_STEAM_MSG_CLOUD_H
