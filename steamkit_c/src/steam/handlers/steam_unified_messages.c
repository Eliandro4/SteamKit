#include "steamkit/steam/handlers/steam_unified_messages.h"
#include "steamkit/steam/handlers/client_msg_handler.h"
#include "steamkit/steam/steam_client.h"
#include "steamkit/steam/handlers/client_msg_protobuf.h"
#include "steamkit/utils/debug_log.h"
#include "steammessages_base.pb-c.h"
#include "steammessages_unified_base.steamclient.pb-c.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct sk_unified_pending_request {
    char* job_name;
    sk_unified_response_fn response_cb;
    void* response_user_data;
    struct sk_unified_pending_request* next;
} sk_unified_pending_request_t;

typedef struct sk_unified_service {
    char* name;
    sk_steam_unified_messages_t* owner;
} sk_unified_service_t;

struct sk_steam_unified_messages {
    struct sk_client_msg_handler base;
    sk_unified_pending_request_t* pending_requests;
    sk_unified_service_t** services;
    size_t service_count;
    size_t service_capacity;
};

static char* sk_strdup(const char* s) {
    if (!s) return NULL;
    size_t len = strlen(s) + 1;
    char* dup = (char*)malloc(len);
    if (dup) memcpy(dup, s, len);
    return dup;
}

static void sk_steam_unified_messages_handle_msg(struct sk_client_msg_handler* handler, const sk_packet_msg_t* packet_msg) {
    if (!handler || !packet_msg) return;
    sk_steam_unified_messages_t* um = (sk_steam_unified_messages_t*)handler;

    uint32_t msg_type = sk_packet_msg_msg_type(packet_msg);
    if (msg_type != SK_EMSG_SERVICE_METHOD_RESPONSE) return;

    size_t data_len = 0;
    const uint8_t* data = sk_packet_msg_data(packet_msg, &data_len);
    if (!data || data_len < 20) return;

    sk_client_msg_protobuf_t* proto = sk_client_msg_protobuf_create_from_packet(packet_msg);
    if (!proto) return;

    CMsgProtoBufHeader* hdr = sk_client_msg_protobuf_header(proto);
    if (!hdr || !hdr->target_job_name) {
        sk_client_msg_protobuf_destroy(proto);
        return;
    }

    const char* target_job_name = hdr->target_job_name;
    uint32_t eresult = hdr->has_eresult ? hdr->eresult : 0;

    sk_unified_pending_request_t* req = um->pending_requests;
    sk_unified_pending_request_t* prev = NULL;
    while (req) {
        if (strcmp(req->job_name, target_job_name) == 0 && req->response_cb) {
            if (prev) prev->next = req->next;
            else um->pending_requests = req->next;
            const uint8_t* body = NULL;
            size_t body_len = 0;
            if (proto) {
                body = sk_client_msg_protobuf_get_body(proto, &body_len);
            }
            req->response_cb(req->response_user_data, body, body_len, eresult);
            free(req->job_name);
            free(req);
            break;
        }
        prev = req;
        req = req->next;
    }

    sk_client_msg_protobuf_destroy(proto);
}

sk_steam_unified_messages_t* sk_steam_unified_messages_create(void) {
    sk_steam_unified_messages_t* um = (sk_steam_unified_messages_t*)calloc(1, sizeof(*um));
    if (um) {
        um->base.handle_msg = sk_steam_unified_messages_handle_msg;
        um->base.handler_type = SK_HANDLER_STEAM_UNIFIED_MESSAGES;
    }
    return um;
}

void sk_steam_unified_messages_destroy(sk_steam_unified_messages_t* um) {
    if (!um) return;
    sk_unified_pending_request_t* req = um->pending_requests;
    while (req) {
        sk_unified_pending_request_t* next = req->next;
        free(req->job_name);
        free(req);
        req = next;
    }
    for (size_t i = 0; i < um->service_count; ++i) {
        free(um->services[i]->name);
        free(um->services[i]);
    }
    free(um->services);
    free(um);
}

sk_unified_service_t* sk_steam_unified_messages_create_service(sk_steam_unified_messages_t* um, const char* service_name) {
    if (!um || !service_name) return NULL;
    for (size_t i = 0; i < um->service_count; ++i) {
        if (strcmp(um->services[i]->name, service_name) == 0) {
            return um->services[i];
        }
    }
    if (um->service_count >= um->service_capacity) {
        size_t new_cap = um->service_capacity == 0 ? 4 : um->service_capacity * 2;
        sk_unified_service_t** new_services = (sk_unified_service_t**)realloc(um->services, new_cap * sizeof(*new_services));
        if (!new_services) return NULL;
        um->services = new_services;
        um->service_capacity = new_cap;
    }
    sk_unified_service_t* svc = (sk_unified_service_t*)calloc(1, sizeof(*svc));
    if (!svc) return NULL;
    svc->name = sk_strdup(service_name);
    svc->owner = um;
    um->services[um->service_count++] = svc;
    return svc;
}

void sk_steam_unified_messages_remove_service(sk_steam_unified_messages_t* um, const char* service_name) {
    if (!um || !service_name) return;
    for (size_t i = 0; i < um->service_count; ++i) {
        if (strcmp(um->services[i]->name, service_name) == 0) {
            free(um->services[i]->name);
            free(um->services[i]);
            memmove(&um->services[i], &um->services[i + 1], (um->service_count - i - 1) * sizeof(*um->services));
            um->service_count--;
            return;
        }
    }
}

void sk_steam_unified_messages_send_request(sk_steam_unified_messages_t* um,
                                            sk_unified_service_t* service,
                                            const char* method_name,
                                            const uint8_t* request_body,
                                            size_t request_body_len,
                                            uint32_t routing_appid,
                                            sk_unified_response_fn response_cb,
                                            void* response_user_data) {
    if (!um || !service || !method_name || !response_cb) return;

    size_t method_len = strlen(method_name);
    size_t job_name_len = strlen(service->name) + 1 + method_len + 1;
    char* job_name = (char*)malloc(job_name_len);
    if (!job_name) return;
    snprintf(job_name, job_name_len, "%s.%s#1", service->name, method_name);

    sk_client_msg_protobuf_t* msg = sk_client_msg_protobuf_create(SK_EMSG_SERVICE_METHOD_CALL_FROM_CLIENT);
    if (!msg) {
        free(job_name);
        return;
    }

    CMsgProtoBufHeader* hdr = sk_client_msg_protobuf_header(msg);
    if (hdr) {
        hdr->target_job_name = job_name;
        hdr->routing_appid = routing_appid;
        hdr->has_routing_appid = 1;
    }

    if (request_body && request_body_len > 0) {
        sk_client_msg_protobuf_set_body(msg, request_body, request_body_len);
    }

    sk_packet_msg_t* pkt = sk_packet_msg_create_from_client_msg_protobuf(msg);
    if (!pkt) {
        sk_client_msg_protobuf_destroy(msg);
        free(job_name);
        return;
    }

    sk_unified_pending_request_t* pending = (sk_unified_pending_request_t*)calloc(1, sizeof(*pending));
    if (!pending) {
        sk_packet_msg_destroy(pkt);
        sk_client_msg_protobuf_destroy(msg);
        free(job_name);
        return;
    }
    pending->job_name = job_name;
    pending->response_cb = response_cb;
    pending->response_user_data = response_user_data;
    pending->next = um->pending_requests;
    um->pending_requests = pending;

    if (um->base.client) {
        sk_steam_client_send(um->base.client, pkt);
    }

    sk_packet_msg_destroy(pkt);
    sk_client_msg_protobuf_destroy(msg);
}
