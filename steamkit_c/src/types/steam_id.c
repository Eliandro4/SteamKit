#include "steamkit/types/steam_id.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

sk_steam_id_t* sk_steam_id_create(uint64_t steamid) {
    sk_steam_id_t* sid = (sk_steam_id_t*)malloc(sizeof(sk_steam_id_t));
    if (sid) sid->steamid = steamid;
    return sid;
}

sk_steam_id_t* sk_steam_id_create_account(sk_universe_t universe, sk_account_type_t type,
                                           uint32_t account_id, uint32_t instance) {
    sk_steam_id_t* sid = sk_steam_id_create(0);
    if (!sid) return NULL;
    sid->steamid = ((uint64_t)universe << 56) |
                   ((uint64_t)type << 52) |
                   ((uint64_t)instance << 32) |
                   ((uint64_t)account_id);
    return sid;
}

sk_steam_id_t* sk_steam_id_clone(const sk_steam_id_t* sid) {
    return sid ? sk_steam_id_create(sid->steamid) : NULL;
}

void sk_steam_id_destroy(sk_steam_id_t* sid) {
    free(sid);
}

void sk_steam_id_set_from_uint64(sk_steam_id_t* sid, uint64_t steamid) {
    if (sid) sid->steamid = steamid;
}

uint64_t sk_steam_id_to_uint64(const sk_steam_id_t* sid) {
    return sid ? sid->steamid : 0;
}

uint64_t sk_steam_id_static_account_key(const sk_steam_id_t* sid) {
    if (!sid) return 0;
    return ((uint64_t)sk_steam_id_universe(sid) << 56) |
           ((uint64_t)sk_steam_id_type(sid) << 52) |
           sk_steam_id_account_id(sid);
}

uint32_t sk_steam_id_account_id(const sk_steam_id_t* sid) {
    return sid ? (uint32_t)(sid->steamid & 0xFFFFFFFF) : 0;
}

uint32_t sk_steam_id_instance(const sk_steam_id_t* sid) {
    return sid ? (uint32_t)((sid->steamid >> 32) & 0x000FFFFF) : 0;
}

sk_account_type_t sk_steam_id_type(const sk_steam_id_t* sid) {
    return sid ? (sk_account_type_t)((sid->steamid >> 52) & 0xFF) : SK_ACCOUNT_TYPE_INVALID;
}

sk_universe_t sk_steam_id_universe(const sk_steam_id_t* sid) {
    return sid ? (sk_universe_t)((sid->steamid >> 56) & 0xFF) : SK_UNIVERSE_INVALID;
}

bool sk_steam_id_is_valid(const sk_steam_id_t* sid) {
    if (!sid) return false;
    sk_universe_t univ = sk_steam_id_universe(sid);
    sk_account_type_t type = sk_steam_id_type(sid);
    if (univ < SK_UNIVERSE_INVALID || univ > SK_UNIVERSE_DEV) return false;
    if (type < SK_ACCOUNT_TYPE_INVALID || type > SK_ACCOUNT_TYPE_ANON_USER) return false;
    if (type == SK_ACCOUNT_TYPE_INDIVIDUAL && sk_steam_id_account_id(sid) == 0) return false;
    return true;
}

bool sk_steam_id_is_individual(const sk_steam_id_t* sid) {
    return sid && (sk_steam_id_type(sid) == SK_ACCOUNT_TYPE_INDIVIDUAL ||
                   sk_steam_id_type(sid) == SK_ACCOUNT_TYPE_CONSOLE_USER);
}

bool sk_steam_id_is_clan(const sk_steam_id_t* sid) {
    return sid && sk_steam_id_type(sid) == SK_ACCOUNT_TYPE_CLAN;
}

bool sk_steam_id_is_chat(const sk_steam_id_t* sid) {
    return sid && sk_steam_id_type(sid) == SK_ACCOUNT_TYPE_CHAT;
}

bool sk_steam_id_is_game_server(const sk_steam_id_t* sid) {
    return sid && (sk_steam_id_type(sid) == SK_ACCOUNT_TYPE_GAME_SERVER ||
                   sk_steam_id_type(sid) == SK_ACCOUNT_TYPE_ANON_GAME_SERVER);
}

bool sk_steam_id_is_anon(const sk_steam_id_t* sid) {
    return sid && (sk_steam_id_type(sid) == SK_ACCOUNT_TYPE_ANON_USER ||
                   sk_steam_id_type(sid) == SK_ACCOUNT_TYPE_ANON_GAME_SERVER);
}

bool sk_steam_id_is_content_server(const sk_steam_id_t* sid) {
    return sid && sk_steam_id_type(sid) == SK_ACCOUNT_TYPE_CONTENT_SERVER;
}

bool sk_steam_id_is_pending(const sk_steam_id_t* sid) {
    return sid && sk_steam_id_type(sid) == SK_ACCOUNT_TYPE_PENDING;
}

bool sk_steam_id_is_console_user(const sk_steam_id_t* sid) {
    return sid && sk_steam_id_type(sid) == SK_ACCOUNT_TYPE_CONSOLE_USER;
}

bool sk_steam_id_is_unknown(const sk_steam_id_t* sid) {
    return sid && sk_steam_id_type(sid) == SK_ACCOUNT_TYPE_INVALID;
}

bool sk_steam_id_is_group(const sk_steam_id_t* sid) {
    return sk_steam_id_is_clan(sid);
}

bool sk_steam_id_is_lobby(const sk_steam_id_t* sid) {
    if (!sid) return false;
    return sk_steam_id_type(sid) == SK_ACCOUNT_TYPE_CHAT &&
           (sk_steam_id_instance(sid) & 0x000FFFFF) != 0;
}

bool sk_steam_id_equals(const sk_steam_id_t* a, const sk_steam_id_t* b) {
    if (!a && !b) return true;
    if (!a || !b) return false;
    return a->steamid == b->steamid;
}

char* sk_steam_id_to_string(const sk_steam_id_t* sid) {
    if (!sid) return NULL;
    char* str = (char*)malloc(21);
    if (str) snprintf(str, 21, "%llu", (unsigned long long)sid->steamid);
    return str;
}

sk_steam_id_t* sk_steam_id_from_steam2(const char* steam2, sk_universe_t universe) {
    if (!steam2) return NULL;
    int auth_server = 0;
    unsigned long long account_id = 0;
    if (sscanf(steam2, "STEAM_%*d:%d:%llu", &auth_server, &account_id) == 2) {
        uint32_t acc_id = (uint32_t)((account_id << 1) | (uint64_t)auth_server);
        return sk_steam_id_create_account(universe, SK_ACCOUNT_TYPE_INDIVIDUAL, acc_id, 1);
    }
    return NULL;
}

sk_steam_id_t* sk_steam_id_from_steam3(const char* steam3) {
    if (!steam3) return NULL;
    char type;
    int universe = 1, account = 0, instance = 1;
    int matched = sscanf(steam3, "[%c:%d:%d:%d]", &type, &universe, &account, &instance);
    if (matched < 3) return NULL;
    if (matched < 4) instance = 1;
    
    sk_account_type_t atype = SK_ACCOUNT_TYPE_INVALID;
    switch (type) {
        case 'A': atype = SK_ACCOUNT_TYPE_ANON_GAME_SERVER; break;
        case 'G': atype = SK_ACCOUNT_TYPE_GAME_SERVER; break;
        case 'M': atype = SK_ACCOUNT_TYPE_MULTISEAT; break;
        case 'P': atype = SK_ACCOUNT_TYPE_PENDING; break;
        case 'C': atype = SK_ACCOUNT_TYPE_CONTENT_SERVER; break;
        case 'g': atype = SK_ACCOUNT_TYPE_CLAN; break;
        case 'T': atype = SK_ACCOUNT_TYPE_CHAT; break;
        case 'c': atype = SK_ACCOUNT_TYPE_CHAT; break;
        case 'L': atype = SK_ACCOUNT_TYPE_CHAT; break;
        case 'I': atype = SK_ACCOUNT_TYPE_INVALID; break;
        case 'U': atype = SK_ACCOUNT_TYPE_INDIVIDUAL; break;
        case 'a': atype = SK_ACCOUNT_TYPE_ANON_USER; break;
        default: atype = SK_ACCOUNT_TYPE_INVALID; break;
    }
    
    if (type == 'g' || type == 'T' || type == 'c' || type == 'L') {
        instance = 0;
    }
    
    sk_universe_t univ = (sk_universe_t)(universe > SK_UNIVERSE_MAX ? SK_UNIVERSE_PUBLIC : universe);
    return sk_steam_id_create_account(univ, atype, (uint32_t)account, (uint32_t)instance);
}

char* sk_steam_id_render_steam2(const sk_steam_id_t* sid) {
    if (!sid) return NULL;
    uint32_t account_id = sk_steam_id_account_id(sid);
    int y = account_id & 1;
    uint32_t z = account_id >> 1;
    char* str = (char*)malloc(32);
    if (str) {
        snprintf(str, 32, "STEAM_%d:%d:%u", (int)sk_steam_id_universe(sid), y, z);
    }
    return str;
}

char* sk_steam_id_render_steam3(const sk_steam_id_t* sid) {
    if (!sid) return NULL;
    char type_char = 'I';
    uint32_t instance = sk_steam_id_instance(sid);
    
    switch (sk_steam_id_type(sid)) {
        case SK_ACCOUNT_TYPE_ANON_GAME_SERVER: type_char = 'A'; break;
        case SK_ACCOUNT_TYPE_GAME_SERVER:       type_char = 'G'; break;
        case SK_ACCOUNT_TYPE_MULTISEAT:         type_char = 'M'; break;
        case SK_ACCOUNT_TYPE_PENDING:           type_char = 'P'; break;
        case SK_ACCOUNT_TYPE_CONTENT_SERVER:    type_char = 'C'; break;
        case SK_ACCOUNT_TYPE_CLAN:              type_char = 'g'; instance = 0; break;
        case SK_ACCOUNT_TYPE_CHAT:
            type_char = 'T';
            if (instance == 0) type_char = 'g';
            break;
        case SK_ACCOUNT_TYPE_INVALID:           type_char = 'I'; break;
        case SK_ACCOUNT_TYPE_INDIVIDUAL:        type_char = 'U'; break;
        case SK_ACCOUNT_TYPE_ANON_USER:         type_char = 'a'; break;
        case SK_ACCOUNT_TYPE_CONSOLE_USER:      type_char = 'U'; break;
        default: break;
    }
    
    char* str = (char*)malloc(32);
    if (str) {
        if (instance == 0 || sk_steam_id_type(sid) == SK_ACCOUNT_TYPE_CHAT) {
            snprintf(str, 32, "[%c:%d:%u]", type_char,
                     (int)sk_steam_id_universe(sid),
                     sk_steam_id_account_id(sid));
        } else {
            snprintf(str, 32, "[%c:%d:%u:%u]", type_char,
                     (int)sk_steam_id_universe(sid),
                     sk_steam_id_account_id(sid),
                     instance);
        }
    }
    return str;
}
