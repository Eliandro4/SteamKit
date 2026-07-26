#include "steamkit/steam/steam_client/callback_mgr/callback_mgr.h"
#include "steamkit/steam/steam_client.h"
#include "steamkit/steam/callbacks.h"
#include <stdlib.h>
#include <string.h>

typedef struct callback_entry {
    size_t callback_id;
    size_t size;
    void* data;
    struct callback_entry* next;
} callback_entry_t;

struct sk_callback_mgr {
    sk_steam_client_t* client;
    callback_entry_t* queue;
    callback_entry_t* queue_tail;
    size_t next_id;
};

sk_callback_mgr_t* sk_callback_mgr_create(sk_steam_client_t* client) {
    sk_callback_mgr_t* mgr = (sk_callback_mgr_t*)calloc(1, sizeof(sk_callback_mgr_t));
    if (mgr) {
        mgr->client = client;
        mgr->next_id = 1;
    }
    return mgr;
}

void sk_callback_mgr_destroy(sk_callback_mgr_t* mgr) {
    if (!mgr) return;
    callback_entry_t* entry = mgr->queue;
    while (entry) {
        callback_entry_t* next = entry->next;
        free(entry->data);
        free(entry);
        entry = next;
    }
    free(mgr);
}

size_t sk_callback_mgr_register(sk_callback_mgr_t* mgr, size_t callback_size) {
    (void)callback_size;
    if (!mgr) return 0;
    return mgr->next_id++;
}

void sk_callback_mgr_post_callback(sk_callback_mgr_t* mgr, void* callback, size_t callback_id) {
    if (!mgr || !callback) return;

    callback_entry_t* entry = (callback_entry_t*)malloc(sizeof(callback_entry_t));
    if (!entry) return;

    entry->callback_id = callback_id;
    entry->size = 0;
    entry->data = callback;
    entry->next = NULL;

    if (mgr->queue_tail) {
        mgr->queue_tail->next = entry;
    } else {
        mgr->queue = entry;
    }
    mgr->queue_tail = entry;

    if (mgr->client) {
        sk_steam_client_post_callback(mgr->client, (uint32_t)callback_id, 0, callback);
    }
}
