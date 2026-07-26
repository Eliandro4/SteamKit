#include "steamkit/types/depot_manifest.h"
#include "steamkit/utils/crypto_helper.h"
#include "content_manifest.pb-c.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

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

sk_depot_manifest_t* sk_depot_manifest_create(void) {
    sk_depot_manifest_t* manifest = (sk_depot_manifest_t*)calloc(1, sizeof(*manifest));
    return manifest;
}

void sk_depot_manifest_set_metadata(sk_depot_manifest_t* manifest, uint32_t depot_id, uint64_t manifest_gid, uint64_t creation_time) {
    if (!manifest) return;
    manifest->depot_id = depot_id;
    manifest->manifest_gid = manifest_gid;
    manifest->creation_time = creation_time;
}

void sk_depot_manifest_decrypt_filenames(sk_depot_manifest_t* manifest, const uint8_t* depot_key, size_t key_len) {
    if (!manifest || !depot_key || key_len != 32) return;
    if (!manifest->filenames_encrypted) return;

    for (uint32_t i = 0; i < manifest->num_files; ++i) {
        sk_depot_file_t* file = &manifest->files[i];
        if (!file->filename) continue;
        char* decrypted = sk_crypto_aes_cbc_decrypt_string(depot_key, file->filename);
        if (decrypted) {
            free(file->filename);
            file->filename = decrypted;
        }
        if (file->link_target) {
            char* decrypted_link = sk_crypto_aes_cbc_decrypt_string(depot_key, file->link_target);
            if (decrypted_link) {
                free(file->link_target);
                file->link_target = decrypted_link;
            }
        }
    }
    manifest->filenames_encrypted = false;
}

void sk_depot_manifest_destroy(sk_depot_manifest_t* manifest) {
    if (!manifest) return;
    if (manifest->files) {
        for (uint32_t i = 0; i < manifest->num_files; ++i) {
            free(manifest->files[i].filename);
            free(manifest->files[i].link_target);
            free(manifest->files[i].chunks);
            free(manifest->files[i].filename_hash);
            free(manifest->files[i].file_hash);
        }
        free(manifest->files);
    }
    free(manifest);
}

#ifdef SK_ENABLE_ZLIB

int sk_depot_manifest_save_to_file(sk_depot_manifest_t* manifest, const char* filename) {
    if (!manifest || !filename) return -1;

    ContentManifestPayload payload = CONTENT_MANIFEST_PAYLOAD__INIT;
    if (manifest->files) {
        payload.n_mappings = manifest->num_files;
        payload.mappings = (ContentManifestPayload__FileMapping**)calloc(manifest->num_files, sizeof(ContentManifestPayload__FileMapping*));
        if (payload.mappings) {
            for (uint32_t i = 0; i < manifest->num_files; ++i) {
                ContentManifestPayload__FileMapping* mapping = (ContentManifestPayload__FileMapping*)calloc(1, sizeof(ContentManifestPayload__FileMapping));
                if (!mapping) continue;
                content_manifest_payload__file_mapping__init(mapping);
                mapping->filename = sk_strdup(manifest->files[i].filename);
                mapping->has_size = true;
                mapping->size = manifest->files[i].total_size;
                mapping->has_flags = true;
                mapping->flags = manifest->files[i].flags;
                if (manifest->files[i].filename_hash && manifest->files[i].filename_hash_len == 20) {
                    mapping->sha_filename.data = manifest->files[i].filename_hash;
                    mapping->sha_filename.len = 20;
                    mapping->has_sha_filename = true;
                }
                if (manifest->files[i].file_hash && manifest->files[i].file_hash_len == 20) {
                    mapping->sha_content.data = manifest->files[i].file_hash;
                    mapping->sha_content.len = 20;
                    mapping->has_sha_content = true;
                }
                mapping->linktarget = sk_strdup(manifest->files[i].link_target);
                if (manifest->files[i].chunks) {
                    mapping->n_chunks = manifest->files[i].num_chunks;
                    mapping->chunks = (ContentManifestPayload__FileMapping__ChunkData**)calloc(manifest->files[i].num_chunks, sizeof(ContentManifestPayload__FileMapping__ChunkData*));
                    if (mapping->chunks) {
                        for (uint32_t j = 0; j < manifest->files[i].num_chunks; ++j) {
                            ContentManifestPayload__FileMapping__ChunkData* chunk = (ContentManifestPayload__FileMapping__ChunkData*)calloc(1, sizeof(ContentManifestPayload__FileMapping__ChunkData));
                            if (!chunk) continue;
                            content_manifest_payload__file_mapping__chunk_data__init(chunk);
                            chunk->sha.data = manifest->files[i].chunks[j].checksum;
                            chunk->sha.len = 20;
                            chunk->has_sha = true;
                            chunk->has_offset = true;
                            chunk->offset = manifest->files[i].chunks[j].offset;
                            chunk->has_cb_original = true;
                            chunk->cb_original = manifest->files[i].chunks[j].uncompressed_length;
                            chunk->has_cb_compressed = true;
                            chunk->cb_compressed = manifest->files[i].chunks[j].compressed_length;
                            mapping->chunks[j] = chunk;
                        }
                    }
                }
                payload.mappings[i] = mapping;
            }
        }
    }

    size_t proto_len = content_manifest_payload__get_packed_size(&payload);
    uint8_t* proto_buf = (uint8_t*)malloc(proto_len);
    if (!proto_buf) return -1;
    content_manifest_payload__pack(&payload, proto_buf);

    uLongf comp_len = compressBound((uLong)proto_len);
    uint8_t* comp_buf = (uint8_t*)malloc(comp_len);
    if (!comp_buf) {
        free(proto_buf);
        return -1;
    }
    int rc = compress(comp_buf, &comp_len, proto_buf, (uLong)proto_len);
    free(proto_buf);
    if (rc != Z_OK) {
        free(comp_buf);
        return -1;
    }

    FILE* fp = fopen(filename, "wb");
    if (!fp) {
        free(comp_buf);
        return -1;
    }
    fwrite(comp_buf, 1, comp_len, fp);
    fclose(fp);

    size_t sha_len = 0;
    uint8_t* sha = sk_crypto_sha1(comp_buf, comp_len, &sha_len);
    if (sha && sha_len == 20) {
        char sha_path[512];
        snprintf(sha_path, sizeof(sha_path), "%s.sha", filename);
        FILE* sha_fp = fopen(sha_path, "wb");
        if (sha_fp) {
            fwrite(sha, 1, 20, sha_fp);
            fclose(sha_fp);
        }
    }
    free(sha);
    free(comp_buf);

    for (uint32_t i = 0; i < payload.n_mappings; ++i) {
        if (payload.mappings[i]) {
            free(payload.mappings[i]->filename);
            free(payload.mappings[i]->linktarget);
            for (size_t j = 0; j < payload.mappings[i]->n_chunks; ++j) {
                free(payload.mappings[i]->chunks[j]);
            }
            free(payload.mappings[i]->chunks);
            free(payload.mappings[i]);
        }
    }
    free(payload.mappings);

    return 0;
}

sk_depot_manifest_t* sk_depot_manifest_load_from_file(const char* filename) {
    if (!filename) return NULL;

    FILE* fp = fopen(filename, "rb");
    if (!fp) return NULL;
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    if (file_size <= 0) {
        fclose(fp);
        return NULL;
    }
    uint8_t* comp_buf = (uint8_t*)malloc(file_size);
    if (!comp_buf) {
        fclose(fp);
        return NULL;
    }
    fread(comp_buf, 1, file_size, fp);
    fclose(fp);

    char sha_path[512];
    snprintf(sha_path, sizeof(sha_path), "%s.sha", filename);
    FILE* sha_fp = fopen(sha_path, "rb");
    if (sha_fp) {
        uint8_t expected_sha[20];
        if (fread(expected_sha, 1, 20, sha_fp) == 20) {
            size_t sha_len = 0;
            uint8_t* actual_sha = sk_crypto_sha1(comp_buf, file_size, &sha_len);
            if (!actual_sha || sha_len != 20 || memcmp(expected_sha, actual_sha, 20) != 0) {
                free(comp_buf);
                free(actual_sha);
                fclose(sha_fp);
                return NULL;
            }
            free(actual_sha);
        }
        fclose(sha_fp);
    }

    uLongf dest_len = file_size * 4;
    uint8_t* decomp_buf = (uint8_t*)malloc(dest_len);
    if (!decomp_buf) {
        free(comp_buf);
        return NULL;
    }
    int rc = uncompress(decomp_buf, &dest_len, comp_buf, (uLong)file_size);
    free(comp_buf);
    if (rc != Z_OK) {
        free(decomp_buf);
        return NULL;
    }

    ContentManifestPayload* payload = content_manifest_payload__unpack(NULL, dest_len, decomp_buf);
    free(decomp_buf);
    if (!payload) return NULL;

    sk_depot_manifest_t* manifest = (sk_depot_manifest_t*)calloc(1, sizeof(*manifest));
    if (!manifest) {
        content_manifest_payload__free_unpacked(payload, NULL);
        return NULL;
    }

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
    return manifest;
}

#endif
