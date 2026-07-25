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

struct sk_steam_client {
    sk_cm_client_t* base;
    sk_client_msg_handler_t** handlers;
    size_t handler_count;
    size_t handler_capacity;
    sk_job_id_t* next_job_id;
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
    return client;
}

void sk_steam_client_destroy(sk_steam_client_t* client) {
    if (!client) return;
    sk_cm_client_disconnect(client->base, true);
    for (size_t i = 0; i < client->handler_count; ++i) {
        free(client->handlers[i]);
    }
    free(client->handlers);
    free(client->next_job_id);
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
