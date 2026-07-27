#define _GNU_SOURCE
#include "steamkit/steam/cm_client.h"
#include "steamkit/steam/steam_client.h"
#include "steamkit/steam/callbacks.h"
#include "steamkit/steam/steam_client/callback_mgr/callback_mgr.h"
#include "steamkit/steam/handlers/client_msg_handler.h"
#include "steamkit/steam/steam_client/configuration/steam_configuration.h"
#include "steamkit/networking/envelope_encrypted_connection.h"
#include "steamkit/utils/debug_log.h"
#include "steamkit/utils/lancache.h"
#include "steamkit/utils/http_client.h"
#include "steamkit/utils/crypto_helper.h"
#include "steamkit/networking/tcp_connection.h"
#include "steamkit/networking/websocket_connection.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <ctype.h>

#define SK_DEFAULT_CM_WEBSOCKET_HOST "cmp1-sea1.steamserver.net"
#define SK_DEFAULT_CM_WEBSOCKET_PORT 443
#define SK_DEFAULT_CM_TCP_HOST "ext1-sea1.steamserver.net"
#define SK_DEFAULT_CM_TCP_PORT 27017

#define SK_CM_MAX_SERVERS 8

typedef struct {
    sk_cm_server_t servers[SK_CM_MAX_SERVERS];
    size_t count;
} sk_cm_server_list_t;

typedef enum {
    SK_CHANNEL_ENCRYPTION_NONE = 0,
    SK_CHANNEL_ENCRYPTION_CONNECTED = 1,
    SK_CHANNEL_ENCRYPTION_HANDSHAKE = 2
} sk_channel_encryption_state_t;

static const sk_cm_server_list_t sk_default_server_list = {
    .servers = {
        { SK_DEFAULT_CM_TCP_HOST, SK_DEFAULT_CM_TCP_PORT, SK_CM_SERVER_TYPE_TCP },
        { SK_DEFAULT_CM_WEBSOCKET_HOST, SK_DEFAULT_CM_WEBSOCKET_PORT, SK_CM_SERVER_TYPE_WEBSOCKET },
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
    sk_cm_server_list_t server_list;
    bool use_dynamic_discovery;
    uint32_t cell_id;
    sk_channel_encryption_state_t encryption_state;
    uint8_t session_key[32];
    sk_envelope_encrypted_connection_t* envelope;
    bool user_notified_connected;
};

static void sk_cm_client_handle_msg(sk_cm_client_t* client, sk_emsg_t msg, const uint8_t* data, size_t len);

static uint8_t* sk_cm_client_decrypt_payload(sk_cm_client_t* client, const uint8_t* data, size_t len, size_t* out_len) {
    if (!client || !data || len == 0 || !out_len) return NULL;
    sk_envelope_encrypted_connection_t* enc = sk_envelope_encrypted_connection_create(NULL);
    if (!enc) return NULL;
    sk_envelope_encrypted_connection_set_key(enc, client->session_key, sizeof(client->session_key));
    uint8_t* decrypted = sk_envelope_encrypted_connection_decrypt(enc, data, len, out_len);
    sk_envelope_encrypted_connection_destroy(enc);
    return decrypted;
}

static uint8_t* sk_cm_client_encrypt_payload(sk_cm_client_t* client, const uint8_t* data, size_t len, size_t* out_len) {
    if (!client || !data || len == 0 || !out_len) return NULL;
    sk_envelope_encrypted_connection_t* enc = sk_envelope_encrypted_connection_create(NULL);
    if (!enc) return NULL;
    sk_envelope_encrypted_connection_set_key(enc, client->session_key, sizeof(client->session_key));
    uint8_t* encrypted = sk_envelope_encrypted_connection_encrypt(enc, data, len, out_len);
    sk_envelope_encrypted_connection_destroy(enc);
    return encrypted;
}

static void sk_cm_client_net_msg_cb(void* user_data, const uint8_t* data, size_t len, sk_emsg_t msg) {
    sk_cm_client_t* client = (sk_cm_client_t*)user_data;
    if (!client || !client->steam_client || !data || len == 0) return;

    const uint8_t* payload = data;
    size_t payload_len = len;
    uint8_t* decrypted_buf = NULL;
    sk_emsg_t parsed_msg = msg;

    if (client->encryption_state == SK_CHANNEL_ENCRYPTION_CONNECTED) {
        decrypted_buf = sk_cm_client_decrypt_payload(client, data, len, &payload_len);
        if (!decrypted_buf) {
            sk_debug_log_warn("CMClient", "Failed to decrypt incoming packet len=%zu", len);
            return;
        }
        payload = decrypted_buf;
        parsed_msg = SK_EMSG_INVALID;
        if (payload_len >= 4) {
            parsed_msg = (sk_emsg_t)(payload[0] | ((uint32_t)payload[1] << 8) | ((uint32_t)payload[2] << 16) | ((uint32_t)payload[3] << 24));
        }
    }

    sk_debug_log_info("CMClient", "Received msg=%u len=%zu", (unsigned)parsed_msg, payload_len);

    if (parsed_msg == SK_EMSG_CHANNEL_ENCRYPT_REQUEST ||
        parsed_msg == SK_EMSG_CHANNEL_ENCRYPT_RESPONSE ||
        parsed_msg == SK_EMSG_CHANNEL_ENCRYPT_RESULT) {
        sk_cm_client_handle_msg(client, parsed_msg, payload, payload_len);
        free(decrypted_buf);
        return;
    }

    sk_packet_msg_t* pkt = sk_packet_msg_create_from_buffer(payload, payload_len);
    if (pkt) {
        sk_steam_client_dispatch_msg(client->steam_client, pkt);
        sk_packet_msg_destroy(pkt);
    }
    free(decrypted_buf);
}

static void sk_cm_client_connected_cb(void* user_data) {
    sk_cm_client_t* client = (sk_cm_client_t*)user_data;
    sk_debug_log_info("CMClient", "Transport connected, awaiting channel encryption handshake");
    (void)client;
}

static void sk_cm_client_disconnected_cb(void* user_data, bool user_initiated) {
    sk_cm_client_t* client = (sk_cm_client_t*)user_data;
    sk_debug_log_info("CMClient", "Connection disconnected callback, user_initiated=%d", user_initiated);
    sk_cm_client_on_disconnected(client, user_initiated);
}

#ifdef SK_ENABLE_CURL
#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>

static const uint8_t sk_steam_public_key_public[] = {
    0x30,0x81,0x9D,0x30,0x0D,0x06,0x09,0x2A,0x86,0x48,0x86,0xF7,0x0D,0x01,0x01,0x01,
    0x05,0x00,0x03,0x81,0x8B,0x00,0x30,0x81,0x87,0x02,0x81,0x81,0x00,0xDF,0xEC,0x1A,
    0xD6,0x2C,0x10,0x66,0x2C,0x17,0x35,0x3A,0x14,0xB0,0x7C,0x59,0x11,0x7F,0x9D,0xD3,
    0xD8,0x2B,0x7A,0xE3,0xE0,0x15,0xCD,0x19,0x1E,0x46,0xE8,0x7B,0x87,0x74,0xA2,0x18,
    0x46,0x31,0xA9,0x03,0x14,0x79,0x82,0x8E,0xE9,0x45,0xA2,0x49,0x12,0xA9,0x23,0x68,
    0x73,0x89,0xCF,0x69,0xA1,0xB1,0x61,0x46,0xBD,0xC1,0xBE,0xBF,0xD6,0x01,0x1B,0xD8,
    0x81,0xD4,0xDC,0x90,0xFB,0xFE,0x4F,0x52,0x73,0x66,0xCB,0x95,0x70,0xD7,0xC5,0x8E,
    0xBA,0x1C,0x7A,0x33,0x75,0xA1,0x62,0x34,0x46,0xBB,0x60,0xB7,0x80,0x68,0xFA,0x13,
    0xA7,0x7A,0x8A,0x37,0x4B,0x9E,0xC6,0xF4,0x5D,0x5F,0x3A,0x99,0xF9,0x9E,0xC4,0x3A,
    0xE9,0x63,0xA2,0xBB,0x88,0x19,0x28,0xE0,0xE7,0x14,0xC0,0x42,0x89,0x02,0x01,0x11,
};
#endif

static void sk_cm_client_handle_channel_encrypt_request(sk_cm_client_t* client, const uint8_t* data, size_t len) {
    if (!client || !data || len < 24) return;
    fprintf(stderr, "[cm] CHANNEL_ENCRYPT_REQUEST: raw_len=%zu hex=", len);
    for (size_t i = 0; i < len && i < 64; i++) fprintf(stderr, "%02X", data[i]);
    fprintf(stderr, "\n");

    const uint8_t* body = data + 20;
    size_t body_len = len < 20 ? 0 : len - 20;
    fprintf(stderr, "[cm] CHANNEL_ENCRYPT_REQUEST: body_len=%zu body_hex=", body_len);
    for (size_t i = 0; i < body_len && i < 64; i++) fprintf(stderr, "%02X", body[i]);
    fprintf(stderr, "\n");

    uint32_t protocol_version = (uint32_t)body[0] | ((uint32_t)body[1] << 8) | ((uint32_t)body[2] << 16) | ((uint32_t)body[3] << 24);
    int32_t universe = (int32_t)((uint32_t)body[4] | ((uint32_t)body[5] << 8) | ((uint32_t)body[6] << 16) | ((uint32_t)body[7] << 24));

    if (protocol_version != 1 || universe != 1) {
        return;
    }

    const uint8_t* challenge = body + 8;
    if (body_len < 8 + 16) return;

    uint8_t temp_key[32];
    for (int i = 0; i < 32; i++) {
        temp_key[i] = (uint8_t)rand();
    }

    uint8_t blob[48];
    memcpy(blob, temp_key, 32);
    memcpy(blob + 32, challenge, 16);

    size_t encrypted_len = 0;
    uint8_t* encrypted = NULL;
#ifdef SK_ENABLE_CURL
    encrypted = sk_crypto_rsa_encrypt_oaep_sha1(blob, sizeof(blob), sk_steam_public_key_public, sizeof(sk_steam_public_key_public), &encrypted_len);
#endif
    if (!encrypted || encrypted_len == 0) {
        sk_debug_log_warn("CMClient", "RSA encryption failed for CHANNEL_ENCRYPT_RESPONSE");
        return;
    }

    uint32_t key_size = (uint32_t)encrypted_len;
    uint32_t crc = sk_crypto_crc32(encrypted, encrypted_len);

    uint8_t response[4 + 4 + encrypted_len + 4 + 4];
    uint32_t resp_protocol_version = 1;
    memcpy(response, &resp_protocol_version, 4);
    memcpy(response + 4, &key_size, 4);
    memcpy(response + 8, encrypted, encrypted_len);
    memcpy(response + 8 + encrypted_len, &crc, 4);
    uint32_t reserved = 0;
    memcpy(response + 8 + encrypted_len + 4, &reserved, 4);

    sk_packet_msg_t* pkt = sk_packet_msg_create(SK_EMSG_CHANNEL_ENCRYPT_RESPONSE, false);
    if (pkt) {
        sk_packet_msg_set_data(pkt, response, sizeof(response));
        sk_steam_client_send(client->steam_client, pkt);
        sk_debug_log_info("CMClient", "Sent CHANNEL_ENCRYPT_RESPONSE len=%zu", sizeof(response));
        sk_packet_msg_destroy(pkt);
    } else {
        sk_debug_log_warn("CMClient", "Failed to create CHANNEL_ENCRYPT_RESPONSE packet");
    }

    memcpy(client->session_key, temp_key, 32);
    client->encryption_state = SK_CHANNEL_ENCRYPTION_HANDSHAKE;

    free(encrypted);
}

static void sk_cm_client_handle_channel_encrypt_result(sk_cm_client_t* client, const uint8_t* data, size_t len) {
    if (!client || !data || len < 4) return;
    fprintf(stderr, "[cm] CHANNEL_ENCRYPT_RESULT raw_len=%zu\n", len);
    for (size_t i = 0; i < len && i < 32; i++) fprintf(stderr, "%02X", data[i]);
    fprintf(stderr, "\n");

    uint32_t result = (uint32_t)data[0] | ((uint32_t)data[1] << 8) | ((uint32_t)data[2] << 16) | ((uint32_t)data[3] << 24);
    sk_debug_log_info("CMClient", "CHANNEL_ENCRYPT_RESULT result=%u", (unsigned)result);
    if (result == 1) {
        client->encryption_state = SK_CHANNEL_ENCRYPTION_CONNECTED;
        sk_debug_log_info("CMClient", "Channel encryption established");
        sk_cm_client_on_connected(client);
    } else {
        client->encryption_state = SK_CHANNEL_ENCRYPTION_NONE;
    }
}

static void sk_cm_client_handle_msg(sk_cm_client_t* client, sk_emsg_t msg, const uint8_t* data, size_t len) {
    switch (msg) {
        case SK_EMSG_CHANNEL_ENCRYPT_REQUEST:
            sk_cm_client_handle_channel_encrypt_request(client, data, len);
            break;
        case SK_EMSG_CHANNEL_ENCRYPT_RESULT:
            sk_cm_client_handle_channel_encrypt_result(client, data, len);
            break;
        default:
            break;
    }
}

sk_cm_client_t* sk_cm_client_create(sk_steam_configuration_t* config, const char* identifier) {
    if (!config || !identifier) return NULL;
    sk_cm_client_t* client = (sk_cm_client_t*)calloc(1, sizeof(sk_cm_client_t));
    if (!client) return NULL;
    client->config = config;
    size_t len = strlen(identifier) + 1;
    client->identifier = (char*)malloc(len);
    if (client->identifier) memcpy(client->identifier, identifier, len);
    client->server_list.servers[0].host = strdup(SK_DEFAULT_CM_TCP_HOST);
    client->server_list.servers[0].port = SK_DEFAULT_CM_TCP_PORT;
    client->server_list.servers[0].type = SK_CM_SERVER_TYPE_TCP;
    client->server_list.servers[1].host = strdup(SK_DEFAULT_CM_WEBSOCKET_HOST);
    client->server_list.servers[1].port = SK_DEFAULT_CM_WEBSOCKET_PORT;
    client->server_list.servers[1].type = SK_CM_SERVER_TYPE_WEBSOCKET;
    client->server_list.count = 2;
    return client;
}

void sk_cm_client_destroy(sk_cm_client_t* client) {
    if (!client) return;
    sk_cm_client_disconnect(client, true);
    if (client->envelope) {
        sk_envelope_encrypted_connection_destroy(client->envelope);
        client->envelope = NULL;
    }
    for (size_t i = 0; i < client->server_list.count; ++i) {
        free((char*)client->server_list.servers[i].host);
    }
    free(client->identifier);
    free(client->steam_id);
    free(client);
}

bool sk_cm_client_is_encryption_ready(const sk_cm_client_t* client) {
    return client && client->encryption_state == SK_CHANNEL_ENCRYPTION_CONNECTED;
}

void sk_cm_client_send(sk_cm_client_t* client, const uint8_t* data, size_t len) {
    sk_cm_client_send_payload(client, data, len);
}

bool sk_cm_client_is_connected(const sk_cm_client_t* client) {
    return client ? client->connected : false;
}

bool sk_cm_client_is_channel_encrypted(const sk_cm_client_t* client) {
    return client && client->connected && client->encryption_state == SK_CHANNEL_ENCRYPTION_CONNECTED;
}

void sk_cm_client_send_payload(sk_cm_client_t* client, const uint8_t* data, size_t len) {
    if (!client || !client->connection || !data || len == 0) return;

    const uint8_t* send_data = data;
    size_t send_len = len;
    uint8_t* encrypted_buf = NULL;

    if (client->encryption_state == SK_CHANNEL_ENCRYPTION_CONNECTED) {
        encrypted_buf = sk_cm_client_encrypt_payload(client, data, len, &send_len);
        if (!encrypted_buf) {
            sk_debug_log_warn("CMClient", "Failed to encrypt outgoing packet len=%zu", len);
            return;
        }
        send_data = encrypted_buf;
    }

    sk_connection_send(client->connection, send_data, send_len);
    free(encrypted_buf);
}

static bool sk_cm_server_is_lancache(const char* host, uint16_t port) {
    if (!host) return false;
#ifdef SK_ENABLE_CURL
    return sk_lancache_detect(host, port);
#else
    return false;
#endif
}

void sk_cm_client_connect(sk_cm_client_t* client) {
    if (!client || client->connected) return;

    int timeout_ms = 5000;
    if (client->config) {
        timeout_ms = sk_steam_configuration_connection_timeout_ms(client->config);
    }

    const sk_cm_server_list_t* server_list = &client->server_list;
    if (server_list->count == 0) {
        server_list = &sk_default_server_list;
    }

    sk_debug_log_info("CMClient", "Trying %zu servers", server_list->count);

    for (size_t i = 0; i < server_list->count; i++) {
        const sk_cm_server_t* server = &server_list->servers[i];

        if (sk_cm_server_is_lancache(server->host, server->port)) {
            sk_debug_log_info("CMClient", "Skipping Lancache server %s:%u", server->host, server->port);
            continue;
        }

        sk_debug_log_info("CMClient", "Server[%zu]: %s:%u type=%d", i, server->host, server->port, server->type);

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
                sk_debug_log_info("CMClient", "WebSocket connected to %s:%u", server->host, server->port);
                client->connection = (sk_connection_t*)ws;
                return;
            }
            sk_websocket_connection_destroy(ws);
            sk_debug_log_debug("CMClient", "WebSocket connection to %s:%u failed", server->host, server->port);
#else
            (void)timeout_ms;
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
                sk_debug_log_info("CMClient", "TCP connected to %s:%u", server->host, server->port);
                client->connection = (sk_connection_t*)tcp;
                return;
            }
            sk_connection_disconnect((sk_connection_t*)tcp, true);
            sk_connection_destroy((sk_connection_t*)tcp);
            sk_debug_log_debug("CMClient", "TCP connection to %s:%u failed", server->host, server->port);
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
    client->encryption_state = SK_CHANNEL_ENCRYPTION_NONE;
    client->user_notified_connected = false;
    if (client->envelope) {
        sk_envelope_encrypted_connection_destroy(client->envelope);
        client->envelope = NULL;
    }
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
    if (!client || !client->callback_fn || !callback_msg) return;
    client->callback_fn(callback_msg);
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
    if (client->steam_client) {
        sk_connected_callback_t* cb = sk_connected_callback_create();
        if (cb) {
            sk_steam_client_post_callback(client->steam_client, SK_CLIENT_CALLBACK_CONNECTED, 0, cb);
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
    if (client->steam_client) {
        sk_disconnected_callback_t* cb = sk_disconnected_callback_create(user_initiated);
        if (cb) {
            sk_steam_client_post_callback(client->steam_client, SK_CLIENT_CALLBACK_DISCONNECTED, 0, cb);
        }
    }
}

void sk_cm_client_set_server_list(sk_cm_client_t* client, const sk_cm_server_t* servers, size_t count) {
    if (!client || !servers || count == 0 || count > 8) return;
    for (size_t i = 0; i < client->server_list.count; ++i) {
        free((char*)client->server_list.servers[i].host);
    }
    memcpy(&client->server_list.servers, servers, count * sizeof(sk_cm_server_t));
    client->server_list.count = count;
    client->use_dynamic_discovery = false;
    for (size_t i = 0; i < count; ++i) {
        if (servers[i].host) {
            client->server_list.servers[i].host = strdup(servers[i].host);
        }
    }
}

const sk_cm_server_t* sk_cm_client_get_server_list(const sk_cm_client_t* client, size_t* out_count) {
    if (!client) return NULL;
    if (out_count) *out_count = client->server_list.count;
    return client->server_list.servers;
}

void sk_cm_client_on_packet_received(sk_cm_client_t* client, const sk_packet_msg_t* packet_msg) {
    if (!client || !packet_msg || !client->steam_client) return;
    sk_steam_client_dispatch_msg(client->steam_client, packet_msg);
}

void sk_cm_client_set_cell_id(sk_cm_client_t* client, uint32_t cell_id) {
    if (client) client->cell_id = cell_id;
}

uint32_t sk_cm_client_get_cell_id(const sk_cm_client_t* client) {
    return client ? client->cell_id : 0;
}

#ifdef SK_ENABLE_CURL

static bool sk_cm_client_parse_server_list_json(sk_cm_client_t* client, const char* json, size_t len) {
    if (!client || !json || len == 0) return false;

    const char* p = json;
    const char* end = json + len;

    const char* serverlist = strstr(p, "\"serverlist\":[");
    if (!serverlist) return false;
    serverlist += strlen("\"serverlist\":[");

    sk_cm_server_t servers[8];
    size_t count = 0;
    size_t parsed = 0;

    while (parsed < len && count < 8) {
        const char* obj_start = strchr(serverlist, '{');
        if (!obj_start) break;

        const char* endpoint_key = "\"endpoint\":\"";
        const char* endpoint_start = strstr(obj_start, endpoint_key);
        if (!endpoint_start) break;
        endpoint_start += strlen(endpoint_key);

        char endpoint[256] = {0};
        size_t ep_len = 0;
        while (endpoint_start < end && *endpoint_start != '"' && ep_len < sizeof(endpoint) - 1) {
            endpoint[ep_len++] = *endpoint_start++;
        }
        endpoint[ep_len] = '\0';

        const char* type_key = "\"type\":\"";
        const char* type_start = strstr(obj_start, type_key);
        if (!type_start) break;
        type_start += strlen(type_key);

        char type[32] = {0};
        size_t type_len = 0;
        while (type_start < end && *type_start != '"' && type_len < sizeof(type) - 1) {
            type[type_len++] = *type_start++;
        }
        type[type_len] = '\0';

        char host[256] = {0};
        uint16_t port = 0;
        const char* colon = strrchr(endpoint, ':');
        if (colon) {
            size_t host_len = (size_t)(colon - endpoint);
            if (host_len >= sizeof(host)) host_len = sizeof(host) - 1;
            memcpy(host, endpoint, host_len);
            host[host_len] = '\0';
            port = (uint16_t)atoi(colon + 1);
        } else {
            strncpy(host, endpoint, sizeof(host) - 1);
        }

        sk_cm_server_type_t server_type = SK_CM_SERVER_TYPE_TCP;
        if (strcmp(type, "websockets") == 0) {
            server_type = SK_CM_SERVER_TYPE_WEBSOCKET;
        }

        servers[count].host = strdup(host);
        servers[count].port = port;
        servers[count].type = server_type;
        count++;
        parsed = (size_t)(obj_start - json);

        const char* obj_end = strchr(obj_start, '}');
        if (!obj_end) break;
        serverlist = obj_end + 1;
    }

    if (count > 0) {
        sk_cm_client_set_server_list(client, servers, count);
    }

    for (size_t i = 0; i < count; ++i) {
        free((char*)servers[i].host);
    }

    return count > 0;
}

bool sk_cm_client_fetch_server_list(sk_cm_client_t* client, uint32_t cell_id) {
    if (!client) return false;

    sk_http_client_t* http = sk_http_client_create();
    if (!http) return false;

    sk_http_client_set_timeout(http, 5000, 10000);

    char url[256];
    snprintf(url, sizeof(url),
             "https://api.steampowered.com/ISteamDirectory/GetCMListForConnect/v1/?cellid=%u",
             (unsigned)cell_id);

    sk_http_response_t* resp = sk_http_client_get(http, url, NULL);
    if (!resp || resp->status_code != 200 || !resp->body) {
        sk_http_response_destroy(resp);
        sk_http_client_destroy(http);
        return false;
    }

    bool ok = sk_cm_client_parse_server_list_json(client, resp->body, resp->body_len);

    sk_http_response_destroy(resp);
    sk_http_client_destroy(http);
    return ok;
}

#endif
