#ifndef STEAMKIT_STEAM_CALLBACKS_H
#define STEAMKIT_STEAM_CALLBACKS_H

#include <stdint.h>
#include <stdbool.h>
#include "steamkit/types/steam_id.h"
#include "steamkit/types/job_id.h"

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
typedef struct sk_steam_client sk_steam_client_t;

// Callback message base - mirrors C# CallbackMsg
typedef struct sk_callback_msg {
    sk_job_id_t job_id;
} sk_callback_msg_t;

// Creates a new callback message
sk_callback_msg_t* sk_callback_msg_create(void);

// Sets the job ID on a callback
void sk_callback_msg_set_job_id(sk_callback_msg_t* msg, const sk_job_id_t* job_id);

// Gets the job ID from a callback
const sk_job_id_t* sk_callback_msg_job_id(const sk_callback_msg_t* msg);

// Destroys a callback message
void sk_callback_msg_destroy(sk_callback_msg_t* msg);

// Callback types

// Connected callback
typedef struct sk_connected_callback {
    sk_callback_msg_t base;
} sk_connected_callback_t;

sk_connected_callback_t* sk_connected_callback_create(void);

// Disconnected callback
typedef struct sk_disconnected_callback {
    sk_callback_msg_t base;
    bool user_initiated;
} sk_disconnected_callback_t;

sk_disconnected_callback_t* sk_disconnected_callback_create(bool user_initiated);

// Logged on callback
typedef struct sk_logged_on_callback {
    sk_callback_msg_t base;
    int result;
} sk_logged_on_callback_t;

sk_logged_on_callback_t* sk_logged_on_callback_create(int result);

// Persona state callback
typedef struct sk_persona_state_callback {
    sk_callback_msg_t base;
    sk_steam_id_t* steam_id;
    uint32_t persona_state;
} sk_persona_state_callback_t;

sk_persona_state_callback_t* sk_persona_state_callback_create(const sk_steam_id_t* steam_id, uint32_t state);

// Friend relationship callback
typedef struct sk_friend_relationship_callback {
    sk_callback_msg_t base;
    sk_steam_id_t* steam_id;
    uint32_t relationship;
} sk_friend_relationship_callback_t;

sk_friend_relationship_callback_t* sk_friend_relationship_callback_create(const sk_steam_id_t* steam_id, uint32_t relationship);

#ifdef __cplusplus
}
#endif

#endif // STEAMKIT_STEAM_CALLBACKS_H
