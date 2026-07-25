#include "steamkit/types/game_id.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

struct sk_game_id {
    uint64_t data;
};

sk_game_id_t* sk_game_id_create(uint64_t id) {
    sk_game_id_t* gid = (sk_game_id_t*)malloc(sizeof(sk_game_id_t));
    if (gid) gid->data = id;
    return gid;
}

sk_game_id_t* sk_game_id_create_from_app(uint32_t app_id) {
    return sk_game_id_create((uint64_t)app_id);
}

sk_game_id_t* sk_game_id_create_mod(uint32_t app_id, const char* mod_path) {
    sk_game_id_t* gid = sk_game_id_create(0);
    if (!gid) return NULL;
    sk_game_id_set_app_id(gid, app_id);
    sk_game_id_set_app_type(gid, SK_GAME_TYPE_GAME_MOD);
    uint32_t crc = 0;
    if (mod_path) {
        for (size_t i = 0; mod_path[i]; ++i) {
            crc = (crc << 8) ^ ((crc >> 24) ^ (uint8_t)mod_path[i]);
        }
    }
    sk_game_id_set_mod_id(gid, crc);
    return gid;
}

sk_game_id_t* sk_game_id_create_shortcut(const char* exe_path, const char* app_name) {
    sk_game_id_t* gid = sk_game_id_create(0);
    if (!gid) return NULL;
    sk_game_id_set_app_type(gid, SK_GAME_TYPE_SHORTCUT);
    uint32_t crc = 0;
    if (exe_path) {
        for (size_t i = 0; exe_path[i]; ++i) {
            crc = (crc << 8) ^ ((crc >> 24) ^ (uint8_t)exe_path[i]);
        }
    }
    if (app_name) {
        for (size_t i = 0; app_name[i]; ++i) {
            crc = (crc << 8) ^ ((crc >> 24) ^ (uint8_t)app_name[i]);
        }
    }
    sk_game_id_set_mod_id(gid, crc);
    return gid;
}

void sk_game_id_destroy(sk_game_id_t* gid) {
    free(gid);
}

void sk_game_id_set(sk_game_id_t* gid, uint64_t id) {
    if (gid) gid->data = id;
}

uint64_t sk_game_id_to_uint64(const sk_game_id_t* gid) {
    return gid ? gid->data : 0;
}

uint32_t sk_game_id_app_id(const sk_game_id_t* gid) {
    return gid ? (uint32_t)(gid->data & 0xFFFFFF) : 0;
}

void sk_game_id_set_app_id(sk_game_id_t* gid, uint32_t app_id) {
    if (gid) {
        gid->data = (gid->data & ~0xFFFFFFULL) | (uint64_t)app_id;
    }
}

sk_game_type_t sk_game_id_app_type(const sk_game_id_t* gid) {
    return gid ? (sk_game_type_t)((gid->data >> 24) & 0xFF) : SK_GAME_TYPE_APP;
}

void sk_game_id_set_app_type(sk_game_id_t* gid, sk_game_type_t type) {
    if (gid) {
        gid->data = (gid->data & ~(0xFFULL << 24)) | ((uint64_t)type << 24);
    }
}

uint32_t sk_game_id_mod_id(const sk_game_id_t* gid) {
    return gid ? (uint32_t)((gid->data >> 32) & 0xFFFFFFFF) : 0;
}

void sk_game_id_set_mod_id(sk_game_id_t* gid, uint32_t mod_id) {
    if (gid) {
        gid->data = (gid->data & ~(0xFFFFFFFFULL << 32)) | ((uint64_t)mod_id << 32);
        gid->data |= (1ULL << 63);
    }
}

bool sk_game_id_is_mod(const sk_game_id_t* gid) {
    return gid && sk_game_id_app_type(gid) == SK_GAME_TYPE_GAME_MOD;
}

bool sk_game_id_is_shortcut(const sk_game_id_t* gid) {
    return gid && sk_game_id_app_type(gid) == SK_GAME_TYPE_SHORTCUT;
}

bool sk_game_id_is_p2p(const sk_game_id_t* gid) {
    return gid && sk_game_id_app_type(gid) == SK_GAME_TYPE_P2P;
}

bool sk_game_id_is_steam_app(const sk_game_id_t* gid) {
    return gid && sk_game_id_app_type(gid) == SK_GAME_TYPE_APP;
}

bool sk_game_id_equals(const sk_game_id_t* a, const sk_game_id_t* b) {
    if (!a && !b) return true;
    if (!a || !b) return false;
    return a->data == b->data;
}

char* sk_game_id_to_string(const sk_game_id_t* gid) {
    if (!gid) return NULL;
    char* str = (char*)malloc(21);
    if (str) snprintf(str, 21, "%llu", (unsigned long long)gid->data);
    return str;
}

sk_game_id_t* sk_game_id_clone(const sk_game_id_t* gid) {
    return gid ? sk_game_id_create(gid->data) : NULL;
}
