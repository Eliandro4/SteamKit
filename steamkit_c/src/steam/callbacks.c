#include "steamkit/steam/callbacks.h"
#include "steamkit/cdn/cdn_server.h"
#include <stdlib.h>
#include <string.h>

static char* sk_strdup(const char* s) {
    if (!s) return NULL;
    size_t len = strlen(s) + 1;
    char* dup = (char*)malloc(len);
    if (dup) memcpy(dup, s, len);
    return dup;
}

sk_callback_msg_t* sk_callback_msg_create(void) {
    return (sk_callback_msg_t*)calloc(1, sizeof(sk_callback_msg_t));
}

void sk_callback_msg_set_job_id(sk_callback_msg_t* msg, const sk_job_id_t* job_id) {
    if (msg && job_id) {
        msg->job_id.base.value = job_id->base.value;
    }
}

const sk_job_id_t* sk_callback_msg_job_id(const sk_callback_msg_t* msg) {
    return msg ? &msg->job_id : NULL;
}

void sk_callback_msg_destroy(sk_callback_msg_t* msg) {
    free(msg);
}

sk_connected_callback_t* sk_connected_callback_create(void) {
    return (sk_connected_callback_t*)calloc(1, sizeof(sk_connected_callback_t));
}

sk_disconnected_callback_t* sk_disconnected_callback_create(bool user_initiated) {
    sk_disconnected_callback_t* cb = (sk_disconnected_callback_t*)calloc(1, sizeof(sk_disconnected_callback_t));
    if (cb) cb->user_initiated = user_initiated;
    return cb;
}

sk_logged_on_callback_t* sk_logged_on_callback_create(int result) {
    sk_logged_on_callback_t* cb = (sk_logged_on_callback_t*)calloc(1, sizeof(sk_logged_on_callback_t));
    if (cb) cb->result = result;
    return cb;
}

sk_logged_off_callback_t* sk_logged_off_callback_create(int result) {
    sk_logged_off_callback_t* cb = (sk_logged_off_callback_t*)calloc(1, sizeof(sk_logged_off_callback_t));
    if (cb) cb->result = result;
    return cb;
}

sk_session_token_callback_t* sk_session_token_callback_create(uint64_t session_token) {
    sk_session_token_callback_t* cb = (sk_session_token_callback_t*)calloc(1, sizeof(sk_session_token_callback_t));
    if (cb) cb->session_token = session_token;
    return cb;
}

sk_account_info_callback_t* sk_account_info_callback_create(const char* persona_name, const char* country, uint32_t count_authed_computers, uint32_t account_flags) {
    sk_account_info_callback_t* cb = (sk_account_info_callback_t*)calloc(1, sizeof(sk_account_info_callback_t));
    if (cb) {
        cb->persona_name = persona_name ? sk_strdup(persona_name) : NULL;
        cb->country = country ? sk_strdup(country) : NULL;
        cb->count_authed_computers = count_authed_computers;
        cb->account_flags = account_flags;
    }
    return cb;
}

void sk_logged_off_callback_destroy(sk_logged_off_callback_t* cb) {
    if (!cb) return;
    free(cb);
}

void sk_session_token_callback_destroy(sk_session_token_callback_t* cb) {
    if (!cb) return;
    free(cb);
}

void sk_account_info_callback_destroy(sk_account_info_callback_t* cb) {
    if (!cb) return;
    free(cb->persona_name);
    free(cb->country);
    free(cb);
}

sk_persona_state_callback_t* sk_persona_state_callback_create(const sk_steam_id_t* steam_id, uint32_t state) {
    sk_persona_state_callback_t* cb = (sk_persona_state_callback_t*)calloc(1, sizeof(sk_persona_state_callback_t));
    if (cb) {
        cb->steam_id = steam_id ? sk_steam_id_clone(steam_id) : NULL;
        cb->persona_state = state;
    }
    return cb;
}

void sk_persona_state_callback_destroy(sk_persona_state_callback_t* cb) {
    if (!cb) return;
    free(cb->steam_id);
    free(cb);
}

sk_friend_relationship_callback_t* sk_friend_relationship_callback_create(const sk_steam_id_t* steam_id, uint32_t relationship) {
    sk_friend_relationship_callback_t* cb = (sk_friend_relationship_callback_t*)calloc(1, sizeof(sk_friend_relationship_callback_t));
    if (cb) {
        cb->steam_id = steam_id ? sk_steam_id_clone(steam_id) : NULL;
        cb->relationship = relationship;
    }
    return cb;
}

void sk_friend_relationship_callback_destroy(sk_friend_relationship_callback_t* cb) {
    if (!cb) return;
    free(cb->steam_id);
    free(cb);
}

sk_license_list_callback_t* sk_license_list_callback_create(uint32_t result, const uint32_t* package_ids, uint32_t num_packages) {
    sk_license_list_callback_t* cb = (sk_license_list_callback_t*)calloc(1, sizeof(*cb));
    if (cb) {
        cb->result = result;
        cb->num_packages = num_packages;
        if (num_packages > 0 && package_ids) {
            cb->package_ids = (uint32_t*)malloc(num_packages * sizeof(uint32_t));
            if (cb->package_ids) memcpy(cb->package_ids, package_ids, num_packages * sizeof(uint32_t));
        }
    }
    return cb;
}

void sk_license_list_callback_destroy(sk_license_list_callback_t* cb) {
    if (!cb) return;
    free(cb->package_ids);
    free(cb);
}

sk_free_license_callback_t* sk_free_license_callback_create(uint32_t result,
    const uint32_t* granted_apps, uint32_t num_granted_apps,
    const uint32_t* granted_packages, uint32_t num_granted_packages) {
    sk_free_license_callback_t* cb = (sk_free_license_callback_t*)calloc(1, sizeof(*cb));
    if (cb) {
        cb->result = result;
        cb->num_granted_apps = num_granted_apps;
        cb->num_granted_packages = num_granted_packages;
        if (num_granted_apps > 0 && granted_apps) {
            cb->granted_apps = (uint32_t*)malloc(num_granted_apps * sizeof(uint32_t));
            if (cb->granted_apps) memcpy(cb->granted_apps, granted_apps, num_granted_apps * sizeof(uint32_t));
        }
        if (num_granted_packages > 0 && granted_packages) {
            cb->granted_packages = (uint32_t*)malloc(num_granted_packages * sizeof(uint32_t));
            if (cb->granted_packages) memcpy(cb->granted_packages, granted_packages, num_granted_packages * sizeof(uint32_t));
        }
    }
    return cb;
}

void sk_free_license_callback_destroy(sk_free_license_callback_t* cb) {
    if (!cb) return;
    free(cb->granted_apps);
    free(cb->granted_packages);
    free(cb);
}

sk_app_ownership_ticket_callback_t* sk_app_ownership_ticket_callback_create(uint32_t result, uint32_t app_id, const uint8_t* ticket_data, size_t ticket_length) {
    sk_app_ownership_ticket_callback_t* cb = (sk_app_ownership_ticket_callback_t*)calloc(1, sizeof(*cb));
    if (cb) {
        cb->result = result;
        cb->app_id = app_id;
        cb->ticket_length = ticket_length;
        if (ticket_length > 0 && ticket_data) {
            cb->ticket_data = (uint8_t*)malloc(ticket_length);
            if (cb->ticket_data) memcpy(cb->ticket_data, ticket_data, ticket_length);
        }
    }
    return cb;
}

void sk_app_ownership_ticket_callback_destroy(sk_app_ownership_ticket_callback_t* cb) {
    if (!cb) return;
    free(cb->ticket_data);
    free(cb);
}

sk_depot_key_callback_t* sk_depot_key_callback_create(uint32_t result, uint32_t depot_id, const uint8_t* depot_key) {
    sk_depot_key_callback_t* cb = (sk_depot_key_callback_t*)calloc(1, sizeof(*cb));
    if (cb) {
        cb->result = result;
        cb->depot_id = depot_id;
        if (depot_key) memcpy(cb->depot_key, depot_key, sizeof(cb->depot_key));
    }
    return cb;
}

void sk_depot_key_callback_destroy(sk_depot_key_callback_t* cb) {
    free(cb);
}

sk_pics_access_token_callback_t* sk_pics_access_token_callback_create(void) {
    return (sk_pics_access_token_callback_t*)calloc(1, sizeof(sk_pics_access_token_callback_t));
}

void sk_pics_access_token_callback_destroy(sk_pics_access_token_callback_t* cb) {
    if (!cb) return;
    free(cb->package_denied_tokens);
    free(cb->app_denied_tokens);
    free(cb->package_tokens);
    free(cb->package_token_appids);
    free(cb->app_tokens);
    free(cb->app_token_appids);
    free(cb);
}

sk_pics_changes_callback_t* sk_pics_changes_callback_create(void) {
    return (sk_pics_changes_callback_t*)calloc(1, sizeof(sk_pics_changes_callback_t));
}

void sk_pics_changes_callback_destroy(sk_pics_changes_callback_t* cb) {
    if (!cb) return;
    free(cb->package_changes);
    free(cb->package_change_numbers);
    free(cb->package_needs_token);
    free(cb->app_changes);
    free(cb->app_change_numbers);
    free(cb->app_needs_token);
    free(cb);
}

sk_pics_product_info_callback_t* sk_pics_product_info_callback_create(void) {
    return (sk_pics_product_info_callback_t*)calloc(1, sizeof(sk_pics_product_info_callback_t));
}

void sk_pics_product_info_callback_destroy(sk_pics_product_info_callback_t* cb) {
    if (!cb) return;
    free(cb->unknown_packages);
    free(cb->unknown_apps);
    free(cb->app_info_ids);
    free(cb->app_info_kv);
    free(cb->package_info_ids);
    free(cb->package_info_kv);
    free(cb);
}

sk_private_beta_callback_t* sk_private_beta_callback_create(uint32_t result, const void* depot_section) {
    sk_private_beta_callback_t* cb = (sk_private_beta_callback_t*)calloc(1, sizeof(*cb));
    if (cb) {
        cb->result = result;
        cb->depot_section = (void*)depot_section;
    }
    return cb;
}

void sk_private_beta_callback_destroy(sk_private_beta_callback_t* cb) {
    free(cb);
}

sk_cdn_server_list_callback_t* sk_cdn_server_list_callback_create(sk_cdn_server_t** servers, uint32_t num_servers) {
    sk_cdn_server_list_callback_t* cb = (sk_cdn_server_list_callback_t*)calloc(1, sizeof(*cb));
    if (cb) {
        cb->num_servers = num_servers;
        if (num_servers > 0 && servers) {
            cb->servers = (sk_cdn_server_t**)malloc(num_servers * sizeof(sk_cdn_server_t*));
            if (cb->servers) {
                for (uint32_t i = 0; i < num_servers; ++i) {
                    cb->servers[i] = servers[i] ? sk_cdn_server_clone(servers[i]) : NULL;
                }
            }
        }
    }
    return cb;
}

void sk_cdn_server_list_callback_destroy(sk_cdn_server_list_callback_t* cb) {
    if (!cb) return;
    for (uint32_t i = 0; i < cb->num_servers; ++i) {
        sk_cdn_server_destroy(cb->servers[i]);
    }
    free(cb->servers);
    free(cb);
}

sk_manifest_request_code_callback_t* sk_manifest_request_code_callback_create(uint32_t result, uint64_t request_code) {
    sk_manifest_request_code_callback_t* cb = (sk_manifest_request_code_callback_t*)calloc(1, sizeof(*cb));
    if (cb) {
        cb->result = result;
        cb->request_code = request_code;
    }
    return cb;
}

void sk_manifest_request_code_callback_destroy(sk_manifest_request_code_callback_t* cb) {
    free(cb);
}

sk_cdn_auth_token_callback_t* sk_cdn_auth_token_callback_create(uint32_t result, const char* token) {
    sk_cdn_auth_token_callback_t* cb = (sk_cdn_auth_token_callback_t*)calloc(1, sizeof(*cb));
    if (cb) {
        cb->result = result;
        cb->token = token ? sk_strdup(token) : NULL;
    }
    return cb;
}

void sk_cdn_auth_token_callback_destroy(sk_cdn_auth_token_callback_t* cb) {
    if (!cb) return;
    free(cb->token);
    free(cb);
}

sk_ugc_details_callback_t* sk_ugc_details_callback_create(uint64_t ugc_id, const char* file_name, const char* url, uint64_t file_size) {
    sk_ugc_details_callback_t* cb = (sk_ugc_details_callback_t*)calloc(1, sizeof(*cb));
    if (cb) {
        cb->ugc_id = ugc_id;
        cb->file_name = file_name ? sk_strdup(file_name) : NULL;
        cb->url = url ? sk_strdup(url) : NULL;
        cb->file_size = file_size;
    }
    return cb;
}

void sk_ugc_details_callback_destroy(sk_ugc_details_callback_t* cb) {
    if (!cb) return;
    free(cb->file_name);
    free(cb->url);
    free(cb);
}
