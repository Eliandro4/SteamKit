#include "steamkit/networking/websocket_connection.h"
#include <stdlib.h>
#include <string.h>

typedef struct sk_websocket_connection {
    sk_connection_t base;
    sk_websocket_context_t context;
    void* http_client;
    bool connected;
} sk_websocket_connection_t;

static char* sk_strdup(const char* s) {
    if (!s) return NULL;
    size_t len = strlen(s) + 1;
    char* dup = (char*)malloc(len);
    if (dup) memcpy(dup, s, len);
    return dup;
}

sk_websocket_connection_t* sk_websocket_connection_create(const sk_websocket_context_t* context) {
    sk_websocket_connection_t* ws = (sk_websocket_connection_t*)calloc(1, sizeof(sk_websocket_connection_t));
    if (!ws) return NULL;
    if (context) {
        ws->base.protocol = SK_PROTOCOL_TYPE_WEBSOCKET;
        ws->base.impl = ws;
        ws->context.url = sk_strdup(context->url);
        ws->context.user_agent = sk_strdup(context->user_agent);
        ws->context.subprotocol_count = context->subprotocol_count;
        if (context->subprotocol_count > 0 && context->subprotocols) {
            ws->context.subprotocols = (const char**)malloc(context->subprotocol_count * sizeof(const char*));
            if (ws->context.subprotocols) {
                for (size_t i = 0; i < context->subprotocol_count; ++i) {
                    ws->context.subprotocols[i] = sk_strdup(context->subprotocols[i]);
                }
            }
        } else {
            ws->context.subprotocols = NULL;
        }
    }
    return ws;
}

void sk_websocket_connection_destroy(sk_websocket_connection_t* ws) {
    if (!ws) return;
    free((void*)ws->context.url);
    if (ws->context.subprotocols) {
        for (size_t i = 0; i < ws->context.subprotocol_count; ++i) {
            free((void*)ws->context.subprotocols[i]);
        }
        free((void*)ws->context.subprotocols);
    }
    free((void*)ws->context.user_agent);
    free(ws);
}
