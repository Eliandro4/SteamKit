#ifndef STEAMKIT_STEAM_HANDLERS_STEAM_PUBLISHED_FILE_H
#define STEAMKIT_STEAM_HANDLERS_STEAM_PUBLISHED_FILE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "steamkit/steam/handlers/client_msg_handler.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sk_steam_published_file sk_steam_published_file_t;

typedef struct sk_published_file_details {
    uint64_t published_file_id;
    uint32_t result;
    char* title;
    char* description;
    uint32_t app_id;
    uint64_t file_size;
    char* preview_url;
    char* file_url;
} sk_published_file_details_t;

sk_steam_published_file_t* sk_steam_published_file_create(void);
void sk_steam_published_file_destroy(sk_steam_published_file_t* pf);

sk_published_file_details_t* sk_steam_published_file_get_details(sk_steam_published_file_t* pf, uint64_t published_file_id);
void sk_published_file_details_destroy(sk_published_file_details_t* details);

#ifdef __cplusplus
}
#endif

#endif // STEAMKIT_STEAM_HANDLERS_STEAM_PUBLISHED_FILE_H
