#define _GNU_SOURCE
#include "steamkit/steam/steam_client.h"
#include "steamkit/steam/cm_client.h"
#include "steamkit/steam/steam_client/configuration/steam_configuration.h"
#include "steamkit/steam/handlers/client_msg_handler.h"
#include "steamkit/base/packet_base.h"
#include "steamkit/utils/debug_log.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

struct sk_callback_entry {
    uint32_t callback_type;
    uint64_t job_id;
    void* data;
    struct sk_callback_entry* next;
};

struct sk_steam_client {
    sk_cm_client_t* base;
    sk_client_msg_handler_t** handlers;
    size_t handler_count;
    size_t handler_capacity;
    sk_job_id_t* next_job_id;

    struct sk_callback_entry* callback_queue;
    struct sk_callback_entry* callback_queue_tail;
    pthread_mutex_t callback_mutex;
    pthread_cond_t callback_cond;
};

sk_steam_client_t* sk_steam_client_create(void) {
    return sk_steam_client_create_with_config(NULL);
}

sk_steam_client_t* sk_steam_client_create_with_config(sk_steam_configuration_t* config) {
    if (!config) {
        config = sk_steam_configuration_create_default();
        if (!config) return NULL;
    }
    sk_steam_client_t* client = (sk_steam_client_t*)calloc(1, sizeof(sk_steam_client_t));
    if (!client) {
        sk_steam_configuration_destroy(config);
        return NULL;
    }
    client->base = sk_cm_client_create(config, "default");
    if (!client->base) {
        free(client);
        sk_steam_configuration_destroy(config);
        return NULL;
    }
    sk_cm_client_set_steam_client(client->base, client);
    client->handler_capacity = 16;
    client->handlers = (struct sk_client_msg_handler**)calloc(client->handler_capacity, sizeof(struct sk_client_msg_handler*));
    if (!client->handlers) {
        sk_cm_client_destroy(client->base);
        free(client);
        sk_steam_configuration_destroy(config);
        return NULL;
    }
    pthread_mutex_init(&client->callback_mutex, NULL);
    pthread_cond_init(&client->callback_cond, NULL);
    return client;
}

void sk_steam_client_destroy(sk_steam_client_t* client) {
    if (!client) return;
    sk_cm_client_disconnect(client->base, true);
    free(client->handlers);
    free(client->next_job_id);

    struct sk_callback_entry* entry = client->callback_queue;
    while (entry) {
        struct sk_callback_entry* next = entry->next;
        free(entry);
        entry = next;
    }
    pthread_mutex_destroy(&client->callback_mutex);
    pthread_cond_destroy(&client->callback_cond);

    sk_cm_client_destroy(client->base);
    free(client);
}

void sk_steam_client_connect(sk_steam_client_t* client) {
    if (!client) return;
    sk_cm_client_connect(client->base);
}

void sk_steam_client_disconnect(sk_steam_client_t* client, bool user_initiated) {
    if (!client) return;
    sk_cm_client_disconnect(client->base, user_initiated);
}

bool sk_steam_client_is_connected(const sk_steam_client_t* client) {
    return client ? sk_cm_client_is_connected(client->base) : false;
}

sk_job_id_t* sk_steam_client_get_next_job_id(sk_steam_client_t* client) {
    if (!client) return NULL;
    static uint64_t sequence = 0;
    sequence++;
    return sk_job_id_create(sequence);
}

void sk_steam_client_log_on(sk_steam_client_t* client, const char* username, const char* password) {
    if (!client || !username) return;
    sk_debug_log_warn("SteamClient", "sk_steam_client_log_on is deprecated. Use sk_steam_user_log_on instead.");
}

void sk_steam_client_log_off(sk_steam_client_t* client) {
    if (!client) return;
    sk_debug_log_warn("SteamClient", "sk_steam_client_log_off is deprecated. Use sk_steam_user_log_off instead.");
}

void sk_steam_client_add_handler(sk_steam_client_t* client, struct sk_client_msg_handler* handler) {
    if (!client || !handler) return;
    if (client->handler_count >= client->handler_capacity) {
        size_t new_cap = client->handler_capacity * 2;
        sk_client_msg_handler_t** new_handlers = (sk_client_msg_handler_t**)realloc(
            client->handlers, new_cap * sizeof(struct sk_client_msg_handler*));
        if (!new_handlers) return;
        client->handlers = new_handlers;
        client->handler_capacity = new_cap;
    }
    client->handlers[client->handler_count++] = handler;
}

void* sk_steam_client_get_handler(const sk_steam_client_t* client, int handler_type) {
    if (!client) return NULL;
    for (size_t i = 0; i < client->handler_count; ++i) {
        if (client->handlers[i] && client->handlers[i]->handler_type == handler_type) {
            return client->handlers[i];
        }
    }
    return NULL;
}

sk_steam_unified_messages_t* sk_steam_client_get_unified_messages(const sk_steam_client_t* client) {
    return (sk_steam_unified_messages_t*)sk_steam_client_get_handler(client, SK_HANDLER_STEAM_UNIFIED_MESSAGES);
}

void sk_steam_client_dispatch_msg(sk_steam_client_t* client, const sk_packet_msg_t* packet_msg) {
    if (!client || !packet_msg) return;
    for (size_t i = 0; i < client->handler_count; ++i) {
        if (client->handlers[i] && client->handlers[i]->handle_msg) {
            client->handlers[i]->handle_msg(client->handlers[i], packet_msg);
        }
    }
}

void sk_steam_client_send(sk_steam_client_t* client, sk_packet_msg_t* packet_msg) {
    if (!client || !packet_msg) return;
    size_t data_len = 0;
    const uint8_t* data = sk_packet_msg_data(packet_msg, &data_len);
    if (data && data_len > 0) {
        sk_connection_send(sk_cm_client_connection(client->base), data, data_len);
    }
}

void sk_steam_client_post_callback(sk_steam_client_t* client, uint32_t callback_type, uint64_t job_id, void* data) {
    if (!client) return;
    struct sk_callback_entry* entry = (struct sk_callback_entry*)calloc(1, sizeof(*entry));
    if (!entry) return;
    entry->callback_type = callback_type;
    entry->job_id = job_id;
    entry->data = data;

    pthread_mutex_lock(&client->callback_mutex);
    if (client->callback_queue_tail) {
        client->callback_queue_tail->next = entry;
    } else {
        client->callback_queue = entry;
    }
    client->callback_queue_tail = entry;
    pthread_cond_signal(&client->callback_cond);
    pthread_mutex_unlock(&client->callback_mutex);
}

void* sk_steam_client_get_next_callback(sk_steam_client_t* client, uint32_t* out_type, uint64_t* out_job_id, int timeout_ms) {
    if (!client) return NULL;
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += timeout_ms / 1000;
    ts.tv_nsec += (long)(timeout_ms % 1000) * 1000000L;
    if (ts.tv_nsec >= 1000000000L) {
        ts.tv_sec += ts.tv_nsec / 1000000000L;
        ts.tv_nsec %= 1000000000L;
    }

    pthread_mutex_lock(&client->callback_mutex);
    while (!client->callback_queue) {
        if (pthread_cond_timedwait(&client->callback_cond, &client->callback_mutex, &ts) != 0) {
            pthread_mutex_unlock(&client->callback_mutex);
            return NULL;
        }
    }
    struct sk_callback_entry* entry = client->callback_queue;
    client->callback_queue = entry->next;
    if (!client->callback_queue) {
        client->callback_queue_tail = NULL;
    }
    pthread_mutex_unlock(&client->callback_mutex);

    if (out_type) *out_type = entry->callback_type;
    if (out_job_id) *out_job_id = entry->job_id;
    void* data = entry->data;
    free(entry);
    return data;
}

void* sk_steam_client_wait_for_job(sk_steam_client_t* client, uint64_t job_id, int timeout_ms) {
    if (!client) return NULL;
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += timeout_ms / 1000;
    ts.tv_nsec += (long)(timeout_ms % 1000) * 1000000L;
    if (ts.tv_nsec >= 1000000000L) {
        ts.tv_sec += ts.tv_nsec / 1000000000L;
        ts.tv_nsec %= 1000000000L;
    }

    pthread_mutex_lock(&client->callback_mutex);
    struct sk_callback_entry* entry = client->callback_queue;
    struct sk_callback_entry* prev = NULL;
    while (entry) {
        if (entry->job_id == job_id) {
            if (prev) {
                prev->next = entry->next;
            } else {
                client->callback_queue = entry->next;
            }
            if (client->callback_queue_tail == entry) {
                client->callback_queue_tail = prev;
            }
            pthread_mutex_unlock(&client->callback_mutex);
            void* data = entry->data;
            free(entry);
            return data;
        }
        prev = entry;
        entry = entry->next;
    }

    while (true) {
        int rc = pthread_cond_timedwait(&client->callback_cond, &client->callback_mutex, &ts);
        if (rc != 0) {
            pthread_mutex_unlock(&client->callback_mutex);
            return NULL;
        }
        entry = client->callback_queue;
        prev = NULL;
        while (entry) {
            if (entry->job_id == job_id) {
                if (prev) {
                    prev->next = entry->next;
                } else {
                    client->callback_queue = entry->next;
                }
                if (client->callback_queue_tail == entry) {
                    client->callback_queue_tail = prev;
                }
                pthread_mutex_unlock(&client->callback_mutex);
                void* data = entry->data;
                free(entry);
                return data;
            }
            prev = entry;
            entry = entry->next;
        }
    }
}

void sk_steam_client_free_callback_data(void* data) {
    free(data);
}
