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
    SK_DEPOT_FILE_FLAG_DIRECTORY = 1
} sk_depot_file_flag_t;

typedef struct sk_depot_file {
    char* filename;
    uint64_t total_size;
    sk_depot_file_flag_t flags;
    sk_depot_chunk_t* chunks;
    uint32_t num_chunks;
} sk_depot_file_t;

typedef struct sk_depot_manifest {
    sk_depot_file_t* files;
    uint32_t num_files;
} sk_depot_manifest_t;

void sk_depot_manifest_decrypt_filenames(sk_depot_manifest_t* manifest, const uint8_t* depot_key, size_t key_len);
void sk_depot_manifest_destroy(sk_depot_manifest_t* manifest);

#ifdef __cplusplus
}
#endif

#endif // STEAMKIT_TYPES_DEPOT_MANIFEST_H
