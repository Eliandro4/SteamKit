#include "steamkit/steam/handlers/steam_apps.h"
#include "steamkit/steam/handlers/client_msg_handler.h"
#include "steamkit/steam/steam_client.h"
#include "steamkit/steam/callbacks.h"
#include "steamkit/utils/debug_log.h"
#include "steamkit/utils/msg_util.h"
#include "steamkit/base/generated/steam_msg_apps.h"
#include "steamkit/steam/handlers/client_msg_protobuf.h"
#include "steammessages_clientserver.pb-c.h"
#include "steammessages_clientserver_2.pb-c.h"
#include "steammessages_clientserver_appinfo.pb-c.h"
#include "steammessages_clientserver_login.pb-c.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

static char* sk_strdup(const char* s) {
    if (!s) return NULL;
    size_t len = strlen(s) + 1;
    char* dup = (char*)malloc(len);
    if (dup) memcpy(dup, s, len);
    return dup;
}

typedef struct sk_steam_apps {
    struct sk_client_msg_handler base;
} sk_steam_apps_t;

static void sk_steam_apps_handle_msg(struct sk_client_msg_handler* handler, const sk_packet_msg_t* packet_msg) {
    if (!handler || !packet_msg) return;
    sk_steam_apps_t* apps = (sk_steam_apps_t*)handler;

    uint32_t msg_type = sk_packet_msg_msg_type(packet_msg);
    sk_client_msg_protobuf_t* proto = sk_client_msg_protobuf_create_from_packet(packet_msg);
    if (!proto) return;

    CMsgProtoBufHeader* hdr = sk_client_msg_protobuf_header(proto);
    uint64_t job_id = hdr && hdr->has_jobid_target ? hdr->jobid_target : 0;

    switch ((sk_emsg_t)msg_type) {
        case SK_EMSG_CLIENT_LICENSE_LIST: {
            size_t data_len = 0;
            const uint8_t* data = sk_packet_msg_data(packet_msg, &data_len);
            CMsgClientLicenseList* license_list = cmsg_client_license_list__unpack(NULL, data_len, data);
            if (license_list) {
                uint32_t* package_ids = NULL;
                uint32_t num_packages = 0;
                if (license_list->licenses) {
                    num_packages = (uint32_t)license_list->n_licenses;
                    package_ids = (uint32_t*)malloc(num_packages * sizeof(uint32_t));
                    if (package_ids) {
                        for (size_t i = 0; i < license_list->n_licenses; ++i) {
                            package_ids[i] = license_list->licenses[i]->package_id;
                        }
                    }
                }
                sk_license_list_callback_t* cb = sk_license_list_callback_create(license_list->eresult, package_ids, num_packages);
                free(package_ids);
                if (apps->base.client) {
                    sk_steam_client_post_callback(apps->base.client, SK_CLIENT_CALLBACK_LICENSE_LIST, 0, cb);
                }
                cmsg_client_license_list__free_unpacked(license_list, NULL);
            }
            break;
        }
        case SK_EMSG_CLIENT_REQUEST_FREE_LICENSE_RESPONSE: {
            size_t data_len = 0;
            const uint8_t* data = sk_packet_msg_data(packet_msg, &data_len);
            CMsgClientRequestFreeLicenseResponse* resp = cmsg_client_request_free_license_response__unpack(NULL, data_len, data);
            if (resp) {
                uint32_t* granted_apps = NULL;
                uint32_t num_apps = 0;
                uint32_t* granted_packages = NULL;
                uint32_t num_packages = 0;
                if (resp->granted_appids) {
                    num_apps = (uint32_t)resp->n_granted_appids;
                    granted_apps = (uint32_t*)malloc(num_apps * sizeof(uint32_t));
                    if (granted_apps) memcpy(granted_apps, resp->granted_appids, num_apps * sizeof(uint32_t));
                }
                if (resp->granted_packageids) {
                    num_packages = (uint32_t)resp->n_granted_packageids;
                    granted_packages = (uint32_t*)malloc(num_packages * sizeof(uint32_t));
                    if (granted_packages) memcpy(granted_packages, resp->granted_packageids, num_packages * sizeof(uint32_t));
                }
                sk_free_license_callback_t* cb = sk_free_license_callback_create(resp->eresult, granted_apps, num_apps, granted_packages, num_packages);
                free(granted_apps);
                free(granted_packages);
                if (apps->base.client) {
                    sk_steam_client_post_callback(apps->base.client, SK_CLIENT_CALLBACK_FREE_LICENSE, job_id, cb);
                }
                cmsg_client_request_free_license_response__free_unpacked(resp, NULL);
            }
            break;
        }
        case SK_EMSG_CLIENT_GET_APP_OWNERSHIP_TICKET_RESPONSE: {
            size_t data_len = 0;
            const uint8_t* data = sk_packet_msg_data(packet_msg, &data_len);
            CMsgClientGetAppOwnershipTicketResponse* resp = cmsg_client_get_app_ownership_ticket_response__unpack(NULL, data_len, data);
            if (resp) {
                sk_app_ownership_ticket_callback_t* cb = sk_app_ownership_ticket_callback_create(resp->eresult, resp->app_id, (const uint8_t*)resp->ticket.data, resp->ticket.len);
                if (apps->base.client) {
                    sk_steam_client_post_callback(apps->base.client, SK_CLIENT_CALLBACK_APP_OWNERSHIP_TICKET, job_id, cb);
                }
                cmsg_client_get_app_ownership_ticket_response__free_unpacked(resp, NULL);
            }
            break;
        }
        case SK_EMSG_CLIENT_GET_DEPOT_DECRYPTION_KEY_RESPONSE: {
            size_t data_len = 0;
            const uint8_t* data = sk_packet_msg_data(packet_msg, &data_len);
            CMsgClientGetDepotDecryptionKeyResponse* resp = cmsg_client_get_depot_decryption_key_response__unpack(NULL, data_len, data);
            if (resp) {
                uint8_t depot_key[32] = {0};
                if (resp->has_depot_encryption_key && resp->depot_encryption_key.data) {
                    size_t copy_len = resp->depot_encryption_key.len < 32 ? resp->depot_encryption_key.len : 32;
                    memcpy(depot_key, resp->depot_encryption_key.data, copy_len);
                }
                sk_depot_key_callback_t* cb = sk_depot_key_callback_create(resp->eresult, resp->depot_id, depot_key);
                if (apps->base.client) {
                    sk_steam_client_post_callback(apps->base.client, SK_CLIENT_CALLBACK_DEPOT_KEY, job_id, cb);
                }
                cmsg_client_get_depot_decryption_key_response__free_unpacked(resp, NULL);
            }
            break;
        }
        case SK_EMSG_CLIENT_PICS_ACCESS_TOKEN_RESPONSE: {
            size_t data_len = 0;
            const uint8_t* data = sk_packet_msg_data(packet_msg, &data_len);
            CMsgClientPICSAccessTokenResponse* resp = cmsg_client_picsaccess_token_response__unpack(NULL, data_len, data);
            if (resp) {
                sk_pics_access_token_callback_t* cb = sk_pics_access_token_callback_create();
                if (cb) {
                    if (resp->package_denied_tokens) {
                        cb->num_package_denied = (uint32_t)resp->n_package_denied_tokens;
                        cb->package_denied_tokens = (uint32_t*)malloc(cb->num_package_denied * sizeof(uint32_t));
                        if (cb->package_denied_tokens) memcpy(cb->package_denied_tokens, resp->package_denied_tokens, cb->num_package_denied * sizeof(uint32_t));
                    }
                    if (resp->app_denied_tokens) {
                        cb->num_app_denied = (uint32_t)resp->n_app_denied_tokens;
                        cb->app_denied_tokens = (uint32_t*)malloc(cb->num_app_denied * sizeof(uint32_t));
                        if (cb->app_denied_tokens) memcpy(cb->app_denied_tokens, resp->app_denied_tokens, cb->num_app_denied * sizeof(uint32_t));
                    }
                    if (resp->package_access_tokens) {
                        cb->num_package_tokens = (uint32_t)resp->n_package_access_tokens;
                        cb->package_tokens = (uint32_t*)malloc(cb->num_package_tokens * sizeof(uint32_t));
                        cb->package_token_appids = (uint32_t*)malloc(cb->num_package_tokens * sizeof(uint32_t));
                        if (cb->package_tokens && cb->package_token_appids) {
                            for (size_t i = 0; i < resp->n_package_access_tokens; ++i) {
                                cb->package_token_appids[i] = resp->package_access_tokens[i]->packageid;
                                cb->package_tokens[i] = resp->package_access_tokens[i]->access_token;
                            }
                        }
                    }
                    if (resp->app_access_tokens) {
                        cb->num_app_tokens = (uint32_t)resp->n_app_access_tokens;
                        cb->app_tokens = (uint32_t*)malloc(cb->num_app_tokens * sizeof(uint32_t));
                        cb->app_token_appids = (uint32_t*)malloc(cb->num_app_tokens * sizeof(uint32_t));
                        if (cb->app_tokens && cb->app_token_appids) {
                            for (size_t i = 0; i < resp->n_app_access_tokens; ++i) {
                                cb->app_token_appids[i] = resp->app_access_tokens[i]->appid;
                                cb->app_tokens[i] = resp->app_access_tokens[i]->access_token;
                            }
                        }
                    }
                }
                if (apps->base.client) {
                    sk_steam_client_post_callback(apps->base.client, SK_CLIENT_CALLBACK_PICS_ACCESS_TOKEN, job_id, cb);
                }
                cmsg_client_picsaccess_token_response__free_unpacked(resp, NULL);
            }
            break;
        }
        case SK_EMSG_CLIENT_PICS_CHANGES_SINCE_RESPONSE: {
            size_t data_len = 0;
            const uint8_t* data = sk_packet_msg_data(packet_msg, &data_len);
            CMsgClientPICSChangesSinceResponse* resp = cmsg_client_picschanges_since_response__unpack(NULL, data_len, data);
            if (resp) {
                sk_pics_changes_callback_t* cb = sk_pics_changes_callback_create();
                if (cb) {
                    cb->since_change_number = resp->since_change_number;
                    cb->current_change_number = resp->current_change_number;
                    cb->requires_full_update = resp->force_full_update;
                    cb->requires_full_app_update = resp->force_full_app_update;
                    cb->requires_full_package_update = resp->force_full_package_update;
                    if (resp->package_changes) {
                        cb->num_package_changes = (uint32_t)resp->n_package_changes;
                        cb->package_changes = (uint32_t*)malloc(cb->num_package_changes * sizeof(uint32_t));
                        cb->package_change_numbers = (uint32_t*)malloc(cb->num_package_changes * sizeof(uint32_t));
                        cb->package_needs_token = (bool*)malloc(cb->num_package_changes * sizeof(bool));
                        if (cb->package_changes && cb->package_change_numbers && cb->package_needs_token) {
                            for (size_t i = 0; i < resp->n_package_changes; ++i) {
                                cb->package_changes[i] = resp->package_changes[i]->packageid;
                                cb->package_change_numbers[i] = resp->package_changes[i]->change_number;
                                cb->package_needs_token[i] = resp->package_changes[i]->needs_token;
                            }
                        }
                    }
                    if (resp->app_changes) {
                        cb->num_app_changes = (uint32_t)resp->n_app_changes;
                        cb->app_changes = (uint32_t*)malloc(cb->num_app_changes * sizeof(uint32_t));
                        cb->app_change_numbers = (uint32_t*)malloc(cb->num_app_changes * sizeof(uint32_t));
                        cb->app_needs_token = (bool*)malloc(cb->num_app_changes * sizeof(bool));
                        if (cb->app_changes && cb->app_change_numbers && cb->app_needs_token) {
                            for (size_t i = 0; i < resp->n_app_changes; ++i) {
                                cb->app_changes[i] = resp->app_changes[i]->appid;
                                cb->app_change_numbers[i] = resp->app_changes[i]->change_number;
                                cb->app_needs_token[i] = resp->app_changes[i]->needs_token;
                            }
                        }
                    }
                }
                if (apps->base.client) {
                    sk_steam_client_post_callback(apps->base.client, SK_CLIENT_CALLBACK_PICS_CHANGES, job_id, cb);
                }
                cmsg_client_picschanges_since_response__free_unpacked(resp, NULL);
            }
            break;
        }
        case SK_EMSG_CLIENT_PICS_PRODUCT_INFO_RESPONSE: {
            size_t data_len = 0;
            const uint8_t* data = sk_packet_msg_data(packet_msg, &data_len);
            CMsgClientPICSProductInfoResponse* resp = cmsg_client_picsproduct_info_response__unpack(NULL, data_len, data);
            if (resp) {
                sk_pics_product_info_callback_t* cb = sk_pics_product_info_callback_create();
                if (cb) {
                    cb->metadata_only = resp->meta_data_only;
                    cb->response_pending = resp->response_pending;
                    if (resp->unknown_packageids) {
                        cb->num_unknown_packages = (uint32_t)resp->n_unknown_packageids;
                        cb->unknown_packages = (uint32_t*)malloc(cb->num_unknown_packages * sizeof(uint32_t));
                        if (cb->unknown_packages) memcpy(cb->unknown_packages, resp->unknown_packageids, cb->num_unknown_packages * sizeof(uint32_t));
                    }
                    if (resp->unknown_appids) {
                        cb->num_unknown_apps = (uint32_t)resp->n_unknown_appids;
                        cb->unknown_apps = (uint32_t*)malloc(cb->num_unknown_apps * sizeof(uint32_t));
                        if (cb->unknown_apps) memcpy(cb->unknown_apps, resp->unknown_appids, cb->num_unknown_apps * sizeof(uint32_t));
                    }
                }
                if (apps->base.client) {
                    sk_steam_client_post_callback(apps->base.client, SK_CLIENT_CALLBACK_PICS_PRODUCT_INFO, job_id, cb);
                }
                cmsg_client_picsproduct_info_response__free_unpacked(resp, NULL);
            }
            break;
        }
        case SK_EMSG_CLIENT_PICS_PRIVATE_BETA_RESPONSE: {
            size_t data_len = 0;
            const uint8_t* data = sk_packet_msg_data(packet_msg, &data_len);
            CMsgClientPICSPrivateBetaResponse* resp = cmsg_client_picsprivate_beta_response__unpack(NULL, data_len, data);
            if (resp) {
                sk_private_beta_callback_t* cb = sk_private_beta_callback_create(resp->eresult, NULL);
                if (apps->base.client) {
                    sk_steam_client_post_callback(apps->base.client, SK_CLIENT_CALLBACK_PRIVATE_BETA, job_id, cb);
                }
                cmsg_client_picsprivate_beta_response__free_unpacked(resp, NULL);
            }
            break;
        }
        default:
            break;
    }

    sk_client_msg_protobuf_destroy(proto);
}

sk_steam_apps_t* sk_steam_apps_create(void) {
    sk_steam_apps_t* apps = (sk_steam_apps_t*)calloc(1, sizeof(sk_steam_apps_t));
    if (apps) {
        apps->base.handle_msg = sk_steam_apps_handle_msg;
        apps->base.handler_type = SK_HANDLER_STEAM_APPS;
    }
    return apps;
}

void sk_steam_apps_destroy(sk_steam_apps_t* apps) {
    free(apps);
}

void sk_steam_apps_request_app_info(sk_steam_apps_t* apps, uint32_t app_id) {
    if (!apps || !apps->base.client) return;

    sk_client_msg_protobuf_t* msg = sk_client_msg_protobuf_create(SK_EMSG_CLIENT_PICS_PRODUCT_INFO_REQUEST);
    if (!msg) return;

    CMsgClientPICSProductInfoRequest req = CMSG_CLIENT_PICSPRODUCT_INFO_REQUEST__INIT;
    CMsgClientPICSProductInfoRequest__AppInfo app_info = CMSG_CLIENT_PICSPRODUCT_INFO_REQUEST__APP_INFO__INIT;
    app_info.appid = app_id;
    app_info.has_appid = true;
    app_info.only_public_obsolete = false;
    app_info.has_only_public_obsolete = true;

    CMsgClientPICSProductInfoRequest__AppInfo* apps_arr[1] = { &app_info };
    req.apps = apps_arr;
    req.n_apps = 1;
    req.meta_data_only = false;
    req.has_meta_data_only = true;

    size_t packed_size = cmsg_client_picsproduct_info_request__get_packed_size(&req);
    uint8_t* packed_buf = (uint8_t*)malloc(packed_size);
    if (packed_buf) {
        cmsg_client_picsproduct_info_request__pack(&req, packed_buf);
        sk_client_msg_protobuf_set_body(msg, packed_buf, packed_size);
        free(packed_buf);
    }

    sk_packet_msg_t* pkt = sk_packet_msg_create_from_client_msg_protobuf(msg);
    if (pkt) {
        sk_steam_client_send(apps->base.client, pkt);
        sk_packet_msg_destroy(pkt);
    }
    sk_client_msg_protobuf_destroy(msg);
}

void sk_steam_apps_request_package_info(sk_steam_apps_t* apps, uint32_t package_id) {
    if (!apps || !apps->base.client) return;

    sk_client_msg_protobuf_t* msg = sk_client_msg_protobuf_create(SK_EMSG_CLIENT_PICS_PRODUCT_INFO_REQUEST);
    if (!msg) return;

    CMsgClientPICSProductInfoRequest req = CMSG_CLIENT_PICSPRODUCT_INFO_REQUEST__INIT;
    CMsgClientPICSProductInfoRequest__PackageInfo pkg_info = CMSG_CLIENT_PICSPRODUCT_INFO_REQUEST__PACKAGE_INFO__INIT;
    pkg_info.packageid = package_id;
    pkg_info.has_packageid = true;

    CMsgClientPICSProductInfoRequest__PackageInfo* pkgs_arr[1] = { &pkg_info };
    req.packages = pkgs_arr;
    req.n_packages = 1;
    req.meta_data_only = false;
    req.has_meta_data_only = true;

    size_t packed_size = cmsg_client_picsproduct_info_request__get_packed_size(&req);
    uint8_t* packed_buf = (uint8_t*)malloc(packed_size);
    if (packed_buf) {
        cmsg_client_picsproduct_info_request__pack(&req, packed_buf);
        sk_client_msg_protobuf_set_body(msg, packed_buf, packed_size);
        free(packed_buf);
    }

    sk_packet_msg_t* pkt = sk_packet_msg_create_from_client_msg_protobuf(msg);
    if (pkt) {
        sk_steam_client_send(apps->base.client, pkt);
        sk_packet_msg_destroy(pkt);
    }
    sk_client_msg_protobuf_destroy(msg);
}

sk_app_ownership_ticket_callback_t* sk_steam_apps_get_app_ownership_ticket(sk_steam_apps_t* apps, uint32_t app_id) {
    if (!apps || !apps->base.client) return NULL;

    sk_client_msg_protobuf_t* msg = sk_client_msg_protobuf_create(SK_EMSG_CLIENT_GET_APP_OWNERSHIP_TICKET);
    if (!msg) return NULL;

    CMsgClientGetAppOwnershipTicket req = CMSG_CLIENT_GET_APP_OWNERSHIP_TICKET__INIT;
    req.app_id = app_id;
    req.has_app_id = true;

    size_t packed_size = cmsg_client_get_app_ownership_ticket__get_packed_size(&req);
    uint8_t* packed_buf = (uint8_t*)malloc(packed_size);
    if (packed_buf) {
        cmsg_client_get_app_ownership_ticket__pack(&req, packed_buf);
        sk_client_msg_protobuf_set_body(msg, packed_buf, packed_size);
        free(packed_buf);
    }

    sk_packet_msg_t* pkt = sk_packet_msg_create_from_client_msg_protobuf(msg);
    if (!pkt) {
        sk_client_msg_protobuf_destroy(msg);
        return NULL;
    }

    CMsgProtoBufHeader* hdr = sk_client_msg_protobuf_header(msg);
    uint64_t job_id = hdr && hdr->has_jobid_target ? hdr->jobid_target : 0;
    if (job_id == 0) {
        sk_job_id_t* job = sk_steam_client_get_next_job_id(apps->base.client);
        if (job) {
            job_id = sk_job_id_value(job);
            sk_job_id_destroy(job);
        }
    }
    sk_packet_msg_set_job_ids(pkt, job_id, 0);

    sk_steam_client_send(apps->base.client, pkt);
    sk_packet_msg_destroy(pkt);
    sk_client_msg_protobuf_destroy(msg);

    sk_debug_log_info("SteamApps", "Requested app ownership ticket for app: %u (job %llu)", app_id, (unsigned long long)job_id);

    sk_app_ownership_ticket_callback_t* result = (sk_app_ownership_ticket_callback_t*)sk_steam_client_wait_for_job(apps->base.client, job_id, 30000);
    if (!result) {
        sk_debug_log_warn("SteamApps", "App ownership ticket timed out for app: %u", app_id);
    }
    return result;
}

sk_free_license_callback_t* sk_steam_apps_request_free_license(sk_steam_apps_t* apps, uint32_t app_id) {
    if (!apps || !apps->base.client) return NULL;

    sk_client_msg_protobuf_t* msg = sk_client_msg_protobuf_create(SK_EMSG_CLIENT_REQUEST_FREE_LICENSE);
    if (!msg) return NULL;

    CMsgClientRequestFreeLicense req = CMSG_CLIENT_REQUEST_FREE_LICENSE__INIT;
    req.appids[0] = app_id;
    req.n_appids = 1;

    size_t packed_size = cmsg_client_request_free_license__get_packed_size(&req);
    uint8_t* packed_buf = (uint8_t*)malloc(packed_size);
    if (packed_buf) {
        cmsg_client_request_free_license__pack(&req, packed_buf);
        sk_client_msg_protobuf_set_body(msg, packed_buf, packed_size);
        free(packed_buf);
    }

    sk_packet_msg_t* pkt = sk_packet_msg_create_from_client_msg_protobuf(msg);
    if (!pkt) {
        sk_client_msg_protobuf_destroy(msg);
        return NULL;
    }

    CMsgProtoBufHeader* hdr = sk_client_msg_protobuf_header(msg);
    uint64_t job_id = hdr && hdr->has_jobid_target ? hdr->jobid_target : 0;
    if (job_id == 0) {
        sk_job_id_t* job = sk_steam_client_get_next_job_id(apps->base.client);
        if (job) {
            job_id = sk_job_id_value(job);
            sk_job_id_destroy(job);
        }
    }
    sk_packet_msg_set_job_ids(pkt, job_id, 0);

    sk_steam_client_send(apps->base.client, pkt);
    sk_packet_msg_destroy(pkt);
    sk_client_msg_protobuf_destroy(msg);

    sk_debug_log_info("SteamApps", "Requested free license for app: %u (job %llu)", app_id, (unsigned long long)job_id);

    sk_free_license_callback_t* result = (sk_free_license_callback_t*)sk_steam_client_wait_for_job(apps->base.client, job_id, 30000);
    if (!result) {
        sk_debug_log_warn("SteamApps", "Free license timed out for app: %u", app_id);
    }
    return result;
}

sk_depot_key_callback_t* sk_steam_apps_get_depot_decryption_key(sk_steam_apps_t* apps, uint32_t depot_id, uint32_t app_id) {
    if (!apps || !apps->base.client) return NULL;

    sk_client_msg_protobuf_t* msg = sk_client_msg_protobuf_create(SK_EMSG_CLIENT_GET_DEPOT_DECRYPTION_KEY);
    if (!msg) return NULL;

    CMsgClientGetDepotDecryptionKey req = CMSG_CLIENT_GET_DEPOT_DECRYPTION_KEY__INIT;
    req.depot_id = depot_id;
    req.has_depot_id = true;
    req.app_id = app_id;
    req.has_app_id = true;

    size_t packed_size = cmsg_client_get_depot_decryption_key__get_packed_size(&req);
    uint8_t* packed_buf = (uint8_t*)malloc(packed_size);
    if (packed_buf) {
        cmsg_client_get_depot_decryption_key__pack(&req, packed_buf);
        sk_client_msg_protobuf_set_body(msg, packed_buf, packed_size);
        free(packed_buf);
    }

    sk_packet_msg_t* pkt = sk_packet_msg_create_from_client_msg_protobuf(msg);
    if (!pkt) {
        sk_client_msg_protobuf_destroy(msg);
        return NULL;
    }

    CMsgProtoBufHeader* hdr = sk_client_msg_protobuf_header(msg);
    uint64_t job_id = hdr && hdr->has_jobid_target ? hdr->jobid_target : 0;
    if (job_id == 0) {
        sk_job_id_t* job = sk_steam_client_get_next_job_id(apps->base.client);
        if (job) {
            job_id = sk_job_id_value(job);
            sk_job_id_destroy(job);
        }
    }
    sk_packet_msg_set_job_ids(pkt, job_id, 0);

    sk_steam_client_send(apps->base.client, pkt);
    sk_packet_msg_destroy(pkt);
    sk_client_msg_protobuf_destroy(msg);

    sk_debug_log_info("SteamApps", "Requested depot decryption key for depot: %u, app: %u (job %llu)", depot_id, app_id, (unsigned long long)job_id);

    sk_depot_key_callback_t* result = (sk_depot_key_callback_t*)sk_steam_client_wait_for_job(apps->base.client, job_id, 30000);
    if (!result) {
        sk_debug_log_warn("SteamApps", "Depot key timed out for depot: %u, app: %u", depot_id, app_id);
    }
    return result;
}

sk_pics_access_token_callback_t* sk_steam_apps_request_pics_access_tokens(sk_steam_apps_t* apps, const uint32_t* app_ids, uint32_t num_apps, const uint32_t* package_ids, uint32_t num_packages) {
    if (!apps || !apps->base.client) return NULL;

    sk_client_msg_protobuf_t* msg = sk_client_msg_protobuf_create(SK_EMSG_CLIENT_PICS_ACCESS_TOKEN_REQUEST);
    if (!msg) return NULL;

    CMsgClientPICSAccessTokenRequest req = CMSG_CLIENT_PICSACCESS_TOKEN_REQUEST__INIT;
    req.appids = (uint32_t*)app_ids;
    req.n_appids = num_apps;
    req.packageids = (uint32_t*)package_ids;
    req.n_packageids = num_packages;

    size_t packed_size = cmsg_client_picsaccess_token_request__get_packed_size(&req);
    uint8_t* packed_buf = (uint8_t*)malloc(packed_size);
    if (packed_buf) {
        cmsg_client_picsaccess_token_request__pack(&req, packed_buf);
        sk_client_msg_protobuf_set_body(msg, packed_buf, packed_size);
        free(packed_buf);
    }

    sk_packet_msg_t* pkt = sk_packet_msg_create_from_client_msg_protobuf(msg);
    if (!pkt) {
        sk_client_msg_protobuf_destroy(msg);
        return NULL;
    }

    CMsgProtoBufHeader* hdr = sk_client_msg_protobuf_header(msg);
    uint64_t job_id = hdr && hdr->has_jobid_target ? hdr->jobid_target : 0;
    if (job_id == 0) {
        sk_job_id_t* job = sk_steam_client_get_next_job_id(apps->base.client);
        if (job) {
            job_id = sk_job_id_value(job);
            sk_job_id_destroy(job);
        }
    }
    sk_packet_msg_set_job_ids(pkt, job_id, 0);

    sk_steam_client_send(apps->base.client, pkt);
    sk_packet_msg_destroy(pkt);
    sk_client_msg_protobuf_destroy(msg);

    sk_debug_log_info("SteamApps", "Requested PICS access tokens for %u apps and %u packages (job %llu)", num_apps, num_packages, (unsigned long long)job_id);

    sk_pics_access_token_callback_t* result = (sk_pics_access_token_callback_t*)sk_steam_client_wait_for_job(apps->base.client, job_id, 30000);
    if (!result) {
        sk_debug_log_warn("SteamApps", "PICS access tokens timed out");
    }
    return result;
}
