#ifndef STEAMKIT_STEAM_STEAM_CLIENT_CALLBACK_MGR_H
#define STEAMKIT_STEAM_STEAM_CLIENT_CALLBACK_MGR_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "steamkit/steam/callbacks.h"

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
typedef struct sk_steam_client sk_steam_client_t;

// Callback manager - mirrors C# CallbackMgr
typedef struct sk_callback_mgr sk_callback_mgr_t;

sk_callback_mgr_t* sk_callback_mgr_create(sk_steam_client_t* client);
void sk_callback_mgr_destroy(sk_callback_mgr_t* mgr);

// Registers a callback type
size_t sk_callback_mgr_register(sk_callback_mgr_t* mgr, size_t callback_size);

// Posts a callback
void sk_callback_mgr_post_callback(sk_callback_mgr_t* mgr, void* callback, size_t callback_id);

#ifdef __cplusplus
}
#endif

#endif // STEAMKIT_STEAM_STEAM_CLIENT_CALLBACK_MGR_H
