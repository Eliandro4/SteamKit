#include "steamkit/steam/callbacks.h"
#include <stdlib.h>
#include <string.h>

sk_callback_msg_t* sk_callback_msg_create(void) {
    return (sk_callback_msg_t*)calloc(1, sizeof(sk_callback_msg_t));
}

void sk_callback_msg_set_job_id(sk_callback_msg_t* msg, const sk_job_id_t* job_id) {
    if (msg && job_id) {
        msg->job_id.base.value = job_id->base.value;
    }
}

const sk_job_id_t* sk_callback_msg_job_id(const sk_callback_msg_t* msg) {
    return msg ? &msg->job_id : NULL;
}

void sk_callback_msg_destroy(sk_callback_msg_t* msg) {
    free(msg);
}

sk_connected_callback_t* sk_connected_callback_create(void) {
    return (sk_connected_callback_t*)calloc(1, sizeof(sk_connected_callback_t));
}

sk_disconnected_callback_t* sk_disconnected_callback_create(bool user_initiated) {
    sk_disconnected_callback_t* cb = (sk_disconnected_callback_t*)calloc(1, sizeof(sk_disconnected_callback_t));
    if (cb) cb->user_initiated = user_initiated;
    return cb;
}

sk_logged_on_callback_t* sk_logged_on_callback_create(int result) {
    sk_logged_on_callback_t* cb = (sk_logged_on_callback_t*)calloc(1, sizeof(sk_logged_on_callback_t));
    if (cb) cb->result = result;
    return cb;
}

sk_persona_state_callback_t* sk_persona_state_callback_create(const sk_steam_id_t* steam_id, uint32_t state) {
    sk_persona_state_callback_t* cb = (sk_persona_state_callback_t*)calloc(1, sizeof(sk_persona_state_callback_t));
    if (cb) {
        cb->steam_id = steam_id ? sk_steam_id_clone(steam_id) : NULL;
        cb->persona_state = state;
    }
    return cb;
}

void sk_persona_state_callback_destroy(sk_persona_state_callback_t* cb) {
    if (!cb) return;
    free(cb->steam_id);
    free(cb);
}

sk_friend_relationship_callback_t* sk_friend_relationship_callback_create(const sk_steam_id_t* steam_id, uint32_t relationship) {
    sk_friend_relationship_callback_t* cb = (sk_friend_relationship_callback_t*)calloc(1, sizeof(sk_friend_relationship_callback_t));
    if (cb) {
        cb->steam_id = steam_id ? sk_steam_id_clone(steam_id) : NULL;
        cb->relationship = relationship;
    }
    return cb;
}

void sk_friend_relationship_callback_destroy(sk_friend_relationship_callback_t* cb) {
    if (!cb) return;
    free(cb->steam_id);
    free(cb);
}
