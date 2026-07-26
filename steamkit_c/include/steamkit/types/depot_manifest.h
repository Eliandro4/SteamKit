#ifndef STEAMKIT_TYPES_DEPOT_MANIFEST_H
#define STEAMKIT_TYPES_DEPOT_MANIFEST_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sk_depot_chunk {
    uint64_t offset;
    uint32_t uncompressed_length;
    uint32_t compressed_length;
    uint8_t checksum[20];
} sk_depot_chunk_t;

typedef enum {
    SK_DEPOT_FILE_FLAG_NONE = 0,
    SK_DEPOT_FILE_FLAG_DIRECTORY = 1,
    SK_DEPOT_FILE_FLAG_EXECUTABLE = 2
} sk_depot_file_flag_t;

typedef struct sk_depot_file {
    char* filename;
    char* link_target;
    uint64_t total_size;
    sk_depot_file_flag_t flags;
    sk_depot_chunk_t* chunks;
    uint32_t num_chunks;
    uint8_t* filename_hash;
    size_t filename_hash_len;
    uint8_t* file_hash;
    size_t file_hash_len;
} sk_depot_file_t;

typedef struct sk_depot_manifest {
    uint32_t depot_id;
    uint64_t manifest_gid;
    uint64_t creation_time;
    bool filenames_encrypted;
    uint32_t encrypted_crc;
    uint64_t total_uncompressed_size;
    uint64_t total_compressed_size;
    sk_depot_file_t* files;
    uint32_t num_files;
} sk_depot_manifest_t;

void sk_depot_manifest_decrypt_filenames(sk_depot_manifest_t* manifest, const uint8_t* depot_key, size_t key_len);
void sk_depot_manifest_destroy(sk_depot_manifest_t* manifest);

sk_depot_manifest_t* sk_depot_manifest_create(void);
void sk_depot_manifest_set_metadata(sk_depot_manifest_t* manifest, uint32_t depot_id, uint64_t manifest_gid, uint64_t creation_time);

#ifdef SK_ENABLE_ZLIB
int sk_depot_manifest_save_to_file(sk_depot_manifest_t* manifest, const char* filename);
sk_depot_manifest_t* sk_depot_manifest_load_from_file(const char* filename);
#endif

#ifdef __cplusplus
}
#endif

#endif // STEAMKIT_TYPES_DEPOT_MANIFEST_H
