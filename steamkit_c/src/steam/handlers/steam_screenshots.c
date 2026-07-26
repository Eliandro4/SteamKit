#include "steamkit/steam/handlers/steam_screenshots.h"
#include "steamkit/steam/handlers/client_msg_handler.h"
#include "steamkit/steam/steam_client.h"
#include "steamkit/steam/callbacks.h"
#include "steamkit/utils/debug_log.h"
#include "steamkit/steam/handlers/client_msg_protobuf.h"
#include "steammessages_clientserver_ucm.pb-c.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <time.h>

typedef struct sk_steam_screenshots {
    struct sk_client_msg_handler base;
} sk_steam_screenshots_t;

static void sk_steam_ss_handle_msg(struct sk_client_msg_handler* handler, const sk_packet_msg_t* packet_msg) {
    if (!handler || !packet_msg) return;
    (void)handler;
    (void)packet_msg;
    sk_debug_log_info("SteamScreenshots", "Received message");
}

typedef struct sk_ss_req_ctx {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    bool done;
    int32_t eresult;
} sk_ss_req_ctx_t;

static void sk_screenshot_response_cb(void* user_data, const uint8_t* body, size_t body_len, uint32_t eresult) {
    sk_ss_req_ctx_t* ctx = (sk_ss_req_ctx_t*)user_data;
    ctx->eresult = eresult;
    pthread_mutex_lock(&ctx->mutex);
    ctx->done = true;
    pthread_cond_signal(&ctx->cond);
    pthread_mutex_unlock(&ctx->mutex);
}

sk_steam_screenshots_t* sk_steam_screenshots_create(void) {
    sk_steam_screenshots_t* screenshots = (sk_steam_screenshots_t*)calloc(1, sizeof(sk_steam_screenshots_t));
    if (screenshots) {
        screenshots->base.handle_msg = sk_steam_ss_handle_msg;
        screenshots->base.handler_type = SK_HANDLER_STEAM_SCREENSHOTS;
    }
    return screenshots;
}

void sk_steam_screenshots_destroy(sk_steam_screenshots_t* screenshots) {
    free(screenshots);
}

void sk_steam_screenshots_take_screenshot(sk_steam_screenshots_t* screenshots) {
    if (!screenshots || !screenshots->base.client) return;

    sk_client_msg_protobuf_t* msg = sk_client_msg_protobuf_create(SK_EMSG_CLIENT_UCM_ADD_SCREENSHOT);
    if (!msg) return;

    CMsgClientUCMAddScreenshot req = CMSG_CLIENT_UCMADD_SCREENSHOT__INIT;
    req.has_appid = true;
    req.appid = 0;
    req.has_rtime32_created = true;
    req.rtime32_created = (uint32_t)time(NULL);
    req.has_permissions = true;
    req.permissions = 0;
    req.has_width = true;
    req.width = 0;
    req.has_height = true;
    req.height = 0;

    size_t packed_size = cmsg_client_ucmadd_screenshot__get_packed_size(&req);
    uint8_t* packed_buf = (uint8_t*)malloc(packed_size);
    if (packed_buf) {
        cmsg_client_ucmadd_screenshot__pack(&req, packed_buf);
        sk_client_msg_protobuf_set_body(msg, packed_buf, packed_size);
        free(packed_buf);
    }

    sk_packet_msg_t* pkt = sk_packet_msg_create_from_client_msg_protobuf(msg);
    if (pkt) {
        sk_steam_client_send(screenshots->base.client, pkt);
        sk_packet_msg_destroy(pkt);
    }

    sk_client_msg_protobuf_destroy(msg);
    sk_debug_log_info("SteamScreenshots", "Take screenshot requested");
}