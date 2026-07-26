#include "steamkit/steam/cm_client.h"
#include "steamkit/steam/steam_client.h"
#include "steamkit/steam/callbacks.h"
#include "steamkit/steam/steam_client/callback_mgr/callback_mgr.h"
#include "steamkit/steam/handlers/client_msg_handler.h"
#include "steamkit/steam/steam_client/configuration/steam_configuration.h"
#include "steamkit/utils/debug_log.h"
#include "steamkit/networking/tcp_connection.h"
#include "steamkit/networking/websocket_connection.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#define SK_DEFAULT_CM_WEBSOCKET_HOST "cmp1-sea1.steamserver.net"
#define SK_DEFAULT_CM_WEBSOCKET_PORT 443
#define SK_DEFAULT_CM_TCP_HOST "ext1-sea1.steamserver.net"
#define SK_DEFAULT_CM_TCP_PORT 27017

typedef enum {
    SK_CM_SERVER_TYPE_WEBSOCKET,
    SK_CM_SERVER_TYPE_TCP,
} sk_cm_server_type_t;

typedef struct {
    const char* host;
    uint16_t port;
    sk_cm_server_type_t type;
} sk_cm_server_t;

typedef struct {
    sk_cm_server_t servers[4];
    size_t count;
} sk_cm_server_list_t;

static const sk_cm_server_list_t sk_default_server_list = {
    .servers = {
        { SK_DEFAULT_CM_WEBSOCKET_HOST, SK_DEFAULT_CM_WEBSOCKET_PORT, SK_CM_SERVER_TYPE_WEBSOCKET },
        { SK_DEFAULT_CM_TCP_HOST, SK_DEFAULT_CM_TCP_PORT, SK_CM_SERVER_TYPE_TCP },
    },
    .count = 2,
};

struct sk_cm_client {
    sk_steam_configuration_t* config;
    char* identifier;
    sk_connection_t* connection;
    sk_steam_id_t* steam_id;
    uint64_t session_token;
    bool connected;
    bool expect_disconnection;
    sk_cm_client_callback_fn callback_fn;
    void* callback_user_data;
    sk_steam_client_t* steam_client;
    const sk_cm_server_list_t* server_list;
};

static void sk_cm_client_net_msg_cb(void* user_data, const uint8_t* data, size_t len, sk_emsg_t msg) {
    sk_cm_client_t* client = (sk_cm_client_t*)user_data;
    if (!client || !client->steam_client) return;
    sk_packet_msg_t* pkt = sk_packet_msg_create_from_buffer(data, len);
    if (pkt) {
        sk_steam_client_dispatch_msg(client->steam_client, pkt);
        sk_packet_msg_destroy(pkt);
    }
}

static void sk_cm_client_connected_cb(void* user_data) {
    sk_cm_client_t* client = (sk_cm_client_t*)user_data;
    sk_cm_client_on_connected(client);
}

static void sk_cm_client_disconnected_cb(void* user_data, bool user_initiated) {
    sk_cm_client_t* client = (sk_cm_client_t*)user_data;
    sk_cm_client_on_disconnected(client, user_initiated);
}

sk_cm_client_t* sk_cm_client_create(sk_steam_configuration_t* config, const char* identifier) {
    if (!config || !identifier) return NULL;
    sk_cm_client_t* client = (sk_cm_client_t*)calloc(1, sizeof(sk_cm_client_t));
    if (!client) return NULL;
    client->config = config;
    size_t len = strlen(identifier) + 1;
    client->identifier = (char*)malloc(len);
    if (client->identifier) memcpy(client->identifier, identifier, len);
    client->server_list = &sk_default_server_list;
    return client;
}

void sk_cm_client_destroy(sk_cm_client_t* client) {
    if (!client) return;
    sk_cm_client_disconnect(client, true);
    free(client->identifier);
    free(client->steam_id);
    free(client);
}

bool sk_cm_client_is_connected(const sk_cm_client_t* client) {
    return client ? client->connected : false;
}

void sk_cm_client_connect(sk_cm_client_t* client) {
    if (!client || client->connected) return;

    int timeout_ms = 5000;
    if (client->config) {
        timeout_ms = sk_steam_configuration_connection_timeout_ms(client->config);
    }

    const sk_cm_server_list_t* server_list = client->server_list;
    if (!server_list || server_list->count == 0) {
        server_list = &sk_default_server_list;
    }

    for (size_t i = 0; i < server_list->count; i++) {
        const sk_cm_server_t* server = &server_list->servers[i];

        if (server->type == SK_CM_SERVER_TYPE_WEBSOCKET) {
#ifdef SK_ENABLE_CURL
            char url[512];
            snprintf(url, sizeof(url), "wss://%s:%u/", server->host, server->port);

            sk_websocket_context_t ws_context = {
                .url = url,
                .subprotocols = NULL,
                .subprotocol_count = 0,
                .user_agent = "steamkit_c/3.0.0",
            };

            sk_websocket_connection_t* ws = sk_websocket_connection_create(&ws_context);
            if (!ws) continue;

            sk_connection_set_callbacks((sk_connection_t*)ws,
                                            sk_cm_client_net_msg_cb,
                                            sk_cm_client_connected_cb,
                                            sk_cm_client_disconnected_cb);
            sk_connection_set_user_data((sk_connection_t*)ws, client);

            sk_websocket_connection_connect(ws, url, timeout_ms);
            if (sk_connection_is_connected((sk_connection_t*)ws)) {
                client->connection = (sk_connection_t*)ws;
                return;
            }
            sk_websocket_connection_destroy(ws);
#endif
        } else {
            sk_tcp_connection_t* tcp = sk_tcp_connection_create();
            if (!tcp) continue;

            sk_connection_set_callbacks((sk_connection_t*)tcp,
                                            sk_cm_client_net_msg_cb,
                                            sk_cm_client_connected_cb,
                                            sk_cm_client_disconnected_cb);
            sk_connection_set_user_data((sk_connection_t*)tcp, client);

            sk_tcp_connection_connect(tcp, server->host, server->port, timeout_ms);
            if (sk_connection_is_connected((sk_connection_t*)tcp)) {
                client->connection = (sk_connection_t*)tcp;
                return;
            }
            sk_connection_destroy((sk_connection_t*)tcp);
        }
    }
}

void sk_cm_client_disconnect(sk_cm_client_t* client, bool user_initiated) {
    if (!client) return;
    if (client->connection) {
        if (sk_connection_protocol_type(client->connection) == SK_PROTOCOL_TYPE_WEBSOCKET) {
            sk_websocket_connection_disconnect((sk_websocket_connection_t*)client->connection, user_initiated);
        } else {
            sk_connection_disconnect(client->connection, user_initiated);
        }
        sk_connection_destroy(client->connection);
        client->connection = NULL;
    }
    bool was_connected = client->connected;
    client->connected = false;
    if (was_connected && client->callback_fn) {
        sk_disconnected_callback_t* cb = sk_disconnected_callback_create(user_initiated);
        if (cb) {
            client->callback_fn(cb);
            sk_callback_msg_destroy((sk_callback_msg_t*)cb);
        }
    }
}

const sk_steam_id_t* sk_cm_client_steam_id(const sk_cm_client_t* client) {
    return client ? client->steam_id : NULL;
}

uint64_t sk_cm_client_session_token(const sk_cm_client_t* client) {
    return client ? client->session_token : 0;
}

const char* sk_cm_client_local_ip(const sk_cm_client_t* client) {
    (void)client;
    return "127.0.0.1";
}

sk_connection_t* sk_cm_client_connection(const sk_cm_client_t* client) {
    return client ? client->connection : NULL;
}

void sk_cm_client_set_callback(sk_cm_client_t* client, sk_cm_client_callback_fn fn, void* user_data) {
    if (client) {
        client->callback_fn = fn;
        client->callback_user_data = user_data;
    }
}

void sk_cm_client_post_callback(sk_cm_client_t* client, void* callback_msg) {
    if (!client || !client->callback_fn) return;
    client->callback_fn(client->callback_user_data);
}

void sk_cm_client_set_steam_client(sk_cm_client_t* client, sk_steam_client_t* steam_client) {
    if (client) {
        client->steam_client = steam_client;
    }
}

sk_steam_client_t* sk_cm_client_get_steam_client(const sk_cm_client_t* client) {
    return client ? client->steam_client : NULL;
}

void sk_cm_client_on_connected(sk_cm_client_t* client) {
    if (!client) return;
    client->connected = true;
    if (client->callback_fn) {
        sk_connected_callback_t* cb = sk_connected_callback_create();
        if (cb) {
            client->callback_fn(cb);
            sk_callback_msg_destroy((sk_callback_msg_t*)cb);
        }
    }
}

void sk_cm_client_on_disconnected(sk_cm_client_t* client, bool user_initiated) {
    if (!client) return;
    client->connected = false;
    if (client->callback_fn) {
        sk_disconnected_callback_t* cb = sk_disconnected_callback_create(user_initiated);
        if (cb) {
            client->callback_fn(cb);
            sk_callback_msg_destroy((sk_callback_msg_t*)cb);
        }
    }
}

void sk_cm_client_on_packet_received(sk_cm_client_t* client, const sk_packet_msg_t* packet_msg) {
    if (!client || !packet_msg || !client->steam_client) return;
    sk_steam_client_dispatch_msg(client->steam_client, packet_msg);
}
