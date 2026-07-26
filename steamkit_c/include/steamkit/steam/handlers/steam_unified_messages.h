#ifndef STEAMKIT_STEAM_HANDLERS_STEAM_UNIFIED_MESSAGES_H
#define STEAMKIT_STEAM_HANDLERS_STEAM_UNIFIED_MESSAGES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "steamkit/steam/handlers/client_msg_handler.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sk_steam_unified_messages sk_steam_unified_messages_t;
typedef struct sk_unified_service sk_unified_service_t;

typedef void (*sk_unified_response_fn)(void* user_data, const uint8_t* body, size_t body_len, uint32_t eresult);

sk_steam_unified_messages_t* sk_steam_unified_messages_create(void);
void sk_steam_unified_messages_destroy(sk_steam_unified_messages_t* um);

sk_unified_service_t* sk_steam_unified_messages_create_service(sk_steam_unified_messages_t* um, const char* service_name);
void sk_steam_unified_messages_remove_service(sk_steam_unified_messages_t* um, const char* service_name);

void sk_steam_unified_messages_send_request(sk_steam_unified_messages_t* um,
                                            sk_unified_service_t* service,
                                            const char* method_name,
                                            const uint8_t* request_body,
                                            size_t request_body_len,
                                            uint32_t routing_appid,
                                            sk_unified_response_fn response_cb,
                                            void* response_user_data);

#ifdef __cplusplus
}
#endif

#endif // STEAMKIT_STEAM_HANDLERS_STEAM_UNIFIED_MESSAGES_H
