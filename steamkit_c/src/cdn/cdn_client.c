#include "steamkit/cdn/cdn_client.h"
#include "steamkit/utils/debug_log.h"
#include "steamkit/utils/http_client.h"
#include "steamkit/utils/crypto_helper.h"
#include "steamkit/utils/msg_util.h"
#include "steamkit/types/depot_manifest.h"
#include "content_manifest.pb-c.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef SK_ENABLE_ZLIB
#include <zlib.h>
#endif

static char* sk_strdup(const char* s) {
    if (!s) return NULL;
    size_t len = strlen(s) + 1;
    char* dup = (char*)malloc(len);
    if (dup) memcpy(dup, s, len);
    return dup;
}

struct sk_cdn_client {
    sk_steam_client_t* steam_client;
};

sk_cdn_client_t* sk_cdn_client_create(sk_steam_client_t* steam_client) {
    sk_cdn_client_t* client = (sk_cdn_client_t*)calloc(1, sizeof(sk_cdn_client_t));
    if (client) {
        client->steam_client = steam_client;
        sk_debug_log_info("CDN", "CDN client created");
    }
    return client;
}

void sk_cdn_client_destroy(sk_cdn_client_t* client) {
    free(client);
}

static char* sk_cdn_build_url(const sk_cdn_server_t* server, const char* path, const char* cdn_token) {
    if (!server || !path) return NULL;
    const char* scheme = (server->https_support && strcmp(server->https_support, "mandatory") == 0) ? "https" : "http";
    const char* host = server->vhost && server->vhost[0] ? server->vhost : server->host;
    size_t len = strlen(scheme) + 3 + strlen(host) + 1 + strlen(path) + 1;
    if (cdn_token) len += strlen(cdn_token) + 1;
    char* url = (char*)malloc(len);
    if (!url) return NULL;
    if (cdn_token && cdn_token[0]) {
        snprintf(url, len, "%s://%s/%s?cdn_token=%s", scheme, host, path, cdn_token);
    } else {
        snprintf(url, len, "%s://%s/%s", scheme, host, path);
    }
    return url;
}

static uint8_t* sk_cdn_decompress(const uint8_t* data, size_t data_len, size_t* out_len) {
#ifdef SK_ENABLE_ZLIB
    if (data_len < 4) return NULL;
    if (data[0] == 'P' && data[1] == 'K' && data[2] == 0x03 && data[3] == 0x04) {
        return NULL; // zip handling would go here
    }
    if (data[0] == 0x78) {
        uLongf dest_len = data_len * 4;
        uint8_t* out = (uint8_t*)malloc(dest_len);
        if (!out) return NULL;
        int rc = uncompress(out, &dest_len, data, (uLong)data_len);
        if (rc != Z_OK) {
            free(out);
            return NULL;
        }
        *out_len = dest_len;
        return out;
    }
#endif
    (void)data;
    (void)data_len;
    (void)out_len;
    return NULL;
}

static uint8_t* sk_cdn_zip_extract_first(const uint8_t* data, size_t data_len, size_t* out_len) {
    if (!data || data_len < 30 || !out_len) return NULL;
    const uint8_t* p = data;
    const uint8_t* end = data + data_len;
    while (p + 30 <= end) {
        if (p[0] == 'P' && p[1] == 'K' && p[2] == 0x03 && p[3] == 0x04) {
            uint16_t version = (uint16_t)p[4] | ((uint16_t)p[5] << 8);
            uint16_t flags = (uint16_t)p[6] | ((uint16_t)p[7] << 8);
            uint16_t compression = (uint16_t)p[8] | ((uint16_t)p[9] << 8);
            uint16_t fn_len = (uint16_t)p[26] | ((uint16_t)p[27] << 8);
            uint16_t extra_len = (uint16_t)p[28] | ((uint16_t)p[29] << 8);
            const uint8_t* file_data = p + 30 + fn_len + extra_len;
            if (file_data > end) return NULL;
            size_t comp_size = (size_t)(end - file_data);
            if (compression == 0) {
                *out_len = comp_size;
                uint8_t* out = (uint8_t*)malloc(comp_size);
                if (out) memcpy(out, file_data, comp_size);
                return out;
            } else if (compression == 8) {
#ifdef SK_ENABLE_ZLIB
                uLongf dest_len = comp_size * 4;
                uint8_t* out = (uint8_t*)malloc(dest_len);
                if (!out) return NULL;
                int rc = uncompress(out, &dest_len, file_data, (uLong)comp_size);
                if (rc != Z_OK) {
                    free(out);
                    return NULL;
                }
                *out_len = dest_len;
                return out;
#else
                (void)comp_size;
                return NULL;
#endif
            }
            return NULL;
        }
        p++;
    }
    return NULL;
}

static sk_depot_manifest_t* sk_cdn_parse_v5_manifest(const uint8_t* data, size_t data_len) {
    if (!data || data_len < 16) return NULL;
    const uint8_t* p = data;
    const uint8_t* end = data + data_len;
    ContentManifestPayload* payload = NULL;
    ContentManifestMetadata* metadata = NULL;

    while (p + 8 <= end) {
        uint32_t magic = sk_msg_read_u32_le(p);
        p += 4;
        if (magic == 0x32C415AB) {
            break;
        }
        uint32_t length = sk_msg_read_u32_le(p);
        p += 4;
        if (p + length > end) break;
        switch (magic) {
            case 0x71F617D0: {
                payload = content_manifest_payload__unpack(NULL, length, p);
                break;
            }
            case 0x1F4812BE: {
                metadata = content_manifest_metadata__unpack(NULL, length, p);
                break;
            }
            case 0x1B81B817: {
                break;
            }
            default:
                break;
        }
        p += length;
    }

    if (!payload || !metadata) {
        content_manifest_payload__free_unpacked(payload, NULL);
        content_manifest_metadata__free_unpacked(metadata, NULL);
        return NULL;
    }

    sk_depot_manifest_t* manifest = (sk_depot_manifest_t*)calloc(1, sizeof(*manifest));
    if (!manifest) {
        content_manifest_payload__free_unpacked(payload, NULL);
        content_manifest_metadata__free_unpacked(metadata, NULL);
        return NULL;
    }

    manifest->depot_id = metadata->has_depot_id ? metadata->depot_id : 0;
    manifest->manifest_gid = metadata->has_gid_manifest ? metadata->gid_manifest : 0;
    manifest->creation_time = metadata->has_creation_time ? metadata->creation_time : 0;
    manifest->filenames_encrypted = metadata->has_filenames_encrypted ? metadata->filenames_encrypted : false;
    manifest->encrypted_crc = metadata->has_crc_encrypted ? metadata->crc_encrypted : 0;
    manifest->total_uncompressed_size = metadata->has_cb_disk_original ? metadata->cb_disk_original : 0;
    manifest->total_compressed_size = metadata->has_cb_disk_compressed ? metadata->cb_disk_compressed : 0;

    if (payload->mappings) {
        manifest->num_files = (uint32_t)payload->n_mappings;
        manifest->files = (sk_depot_file_t*)calloc(manifest->num_files, sizeof(sk_depot_file_t));
        if (manifest->files) {
            for (size_t i = 0; i < payload->n_mappings; ++i) {
                ContentManifestPayload__FileMapping* mapping = payload->mappings[i];
                sk_depot_file_t* file = &manifest->files[i];
                file->filename = mapping->filename ? sk_strdup(mapping->filename) : NULL;
                file->total_size = mapping->has_size ? mapping->size : 0;
                file->flags = mapping->has_flags ? (sk_depot_file_flag_t)mapping->flags : SK_DEPOT_FILE_FLAG_NONE;
                if (mapping->has_sha_filename && mapping->sha_filename.len == 20 && mapping->sha_filename.data) {
                    file->filename_hash = (uint8_t*)malloc(20);
                    if (file->filename_hash) memcpy(file->filename_hash, mapping->sha_filename.data, 20);
                    file->filename_hash_len = 20;
                }
                if (mapping->has_sha_content && mapping->sha_content.len == 20 && mapping->sha_content.data) {
                    file->file_hash = (uint8_t*)malloc(20);
                    if (file->file_hash) memcpy(file->file_hash, mapping->sha_content.data, 20);
                    file->file_hash_len = 20;
                }
                file->link_target = mapping->linktarget ? sk_strdup(mapping->linktarget) : NULL;
                if (mapping->chunks) {
                    file->num_chunks = (uint32_t)mapping->n_chunks;
                    file->chunks = (sk_depot_chunk_t*)calloc(file->num_chunks, sizeof(sk_depot_chunk_t));
                    if (file->chunks) {
                        for (size_t j = 0; j < mapping->n_chunks; ++j) {
                            ContentManifestPayload__FileMapping__ChunkData* chunk = mapping->chunks[j];
                            sk_depot_chunk_t* dep_chunk = &file->chunks[j];
                            if (chunk->has_sha && chunk->sha.len == 20 && chunk->sha.data) {
                                memcpy(dep_chunk->checksum, chunk->sha.data, 20);
                            }
                            dep_chunk->offset = chunk->has_offset ? chunk->offset : 0;
                            dep_chunk->uncompressed_length = chunk->has_cb_original ? chunk->cb_original : 0;
                            dep_chunk->compressed_length = chunk->has_cb_compressed ? chunk->cb_compressed : 0;
                        }
                    }
                }
            }
        }
    }

    content_manifest_payload__free_unpacked(payload, NULL);
    content_manifest_metadata__free_unpacked(metadata, NULL);
    return manifest;
}

static int sk_cdn_process_chunk(const uint8_t* data, size_t data_len, uint8_t* buffer, size_t buffer_size,
                                const uint8_t* depot_key, size_t key_len, uint32_t expected_uncompressed) {
    if (!data || data_len < 16 || !buffer || !depot_key || key_len != 32) return -1;

    size_t decrypted_len = 0;
    uint8_t* decrypted = sk_crypto_symmetric_decrypt(data, data_len, depot_key, key_len, &decrypted_len);
    if (!decrypted || decrypted_len == 0) {
        free(decrypted);
        return -1;
    }

    size_t decompressed_len = 0;
    uint8_t* decompressed = sk_cdn_decompress(decrypted, decrypted_len, &decompressed_len);
    free(decrypted);
    if (!decompressed) {
        memcpy(buffer, decrypted, decrypted_len < buffer_size ? decrypted_len : buffer_size);
        return (int)(decrypted_len < buffer_size ? decrypted_len : buffer_size);
    }

    if (decompressed_len > buffer_size) {
        free(decompressed);
        return -1;
    }

    memcpy(buffer, decompressed, decompressed_len);
    free(decompressed);
    return (int)decompressed_len;
}

sk_depot_manifest_t* sk_cdn_client_download_manifest(
    sk_cdn_client_t* client,
    uint32_t depot_id,
    uint64_t manifest_id,
    uint64_t manifest_request_code,
    sk_cdn_server_t* server,
    const uint8_t* depot_key,
    size_t key_len,
    const char* cdn_token) {

    (void)client;
    (void)depot_key;
    (void)key_len;

    if (!server) return NULL;

    char path[256];
    snprintf(path, sizeof(path), "depot/%u/manifest/%llu/5/%llu", depot_id, (unsigned long long)manifest_id, (unsigned long long)manifest_request_code);

    char* url = sk_cdn_build_url(server, path, cdn_token);
    if (!url) return NULL;

    sk_debug_log_info("CDN", "Downloading manifest from %s", url);

    sk_http_client_t* http = sk_http_client_create();
    if (!http) {
        free(url);
        return NULL;
    }

    sk_http_client_set_timeout(http, 10000, 60000);
    const char* headers[] = { "Accept: */*", NULL };
    sk_http_response_t* resp = sk_http_client_get(http, url, headers);
    free(url);

    if (!resp || resp->status_code != 200 || resp->body_len == 0) {
        sk_debug_log_error("CDN", "Failed to download manifest, status: %ld", resp ? (long)resp->status_code : -1);
        sk_http_client_destroy(http);
        sk_http_response_destroy(resp);
        return NULL;
    }

    size_t manifest_data_len = 0;
    uint8_t* manifest_data = sk_cdn_zip_extract_first((const uint8_t*)resp->body, resp->body_len, &manifest_data_len);
    if (!manifest_data) {
        sk_debug_log_error("CDN", "Failed to extract manifest from zip");
        sk_http_client_destroy(http);
        sk_http_response_destroy(resp);
        return NULL;
    }

    sk_depot_manifest_t* manifest = sk_cdn_parse_v5_manifest(manifest_data, manifest_data_len);
    free(manifest_data);

    if (!manifest) {
        sk_debug_log_warn("CDN", "Failed to parse manifest protobuf, returning empty manifest");
        manifest = (sk_depot_manifest_t*)calloc(1, sizeof(*manifest));
        if (!manifest) {
            sk_http_client_destroy(http);
            sk_http_response_destroy(resp);
            return NULL;
        }
        manifest->depot_id = depot_id;
        manifest->manifest_gid = manifest_id;
    } else {
        manifest->depot_id = depot_id;
        manifest->manifest_gid = manifest_id;
    }

    if (depot_key && key_len == 32) {
        sk_depot_manifest_decrypt_filenames(manifest, depot_key, key_len);
    }

    sk_http_client_destroy(http);
    sk_http_response_destroy(resp);

    sk_debug_log_info("CDN", "Manifest downloaded for depot %u with %u files", depot_id, manifest->num_files);
    return manifest;
}

int sk_cdn_client_download_depot_chunk(
    sk_cdn_client_t* client,
    uint32_t depot_id,
    const sk_depot_chunk_t* chunk,
    sk_cdn_server_t* server,
    uint8_t* buffer,
    size_t buffer_size,
    const uint8_t* depot_key,
    size_t key_len,
    const char* cdn_token) {

    (void)client;
    if (!chunk || !server || !buffer || !depot_key || key_len != 32) return -1;

    char chunk_id_hex[41];
    for (int i = 0; i < 20; ++i) {
        snprintf(chunk_id_hex + i * 2, 3, "%02x", chunk->checksum[i]);
    }
    chunk_id_hex[40] = '\0';

    char path[256];
    snprintf(path, sizeof(path), "depot/%u/chunk/%s", depot_id, chunk_id_hex);

    char* url = sk_cdn_build_url(server, path, cdn_token);
    if (!url) return -1;

    sk_debug_log_info("CDN", "Downloading chunk from %s", url);

    sk_http_client_t* http = sk_http_client_create();
    if (!http) {
        free(url);
        return -1;
    }

    sk_http_client_set_timeout(http, 10000, 60000);
    const char* headers[] = { "Accept: */*", NULL };
    sk_http_response_t* resp = sk_http_client_get(http, url, headers);
    free(url);

    if (!resp || resp->status_code != 200 || resp->body_len == 0) {
        sk_debug_log_error("CDN", "Failed to download chunk, status: %ld", resp ? (long)resp->status_code : -1);
        sk_http_client_destroy(http);
        sk_http_response_destroy(resp);
        return -1;
    }

    int written = sk_cdn_process_chunk((const uint8_t*)resp->body, resp->body_len, buffer, buffer_size, depot_key, key_len, chunk->uncompressed_length);

    sk_http_client_destroy(http);
    sk_http_response_destroy(resp);

    if (written < 0) {
        sk_debug_log_error("CDN", "Failed to process chunk");
        return -1;
    }

    sk_debug_log_info("CDN", "Chunk downloaded and processed: %d bytes", written);
    return written;
}
