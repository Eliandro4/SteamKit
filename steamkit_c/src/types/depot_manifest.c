#include "steamkit/types/depot_manifest.h"
#include <stdlib.h>
#include <string.h>

void sk_depot_manifest_decrypt_filenames(sk_depot_manifest_t* manifest, const uint8_t* depot_key, size_t key_len) {
    (void)manifest;
    (void)depot_key;
    (void)key_len;
    // Stub: Filename decryption would be implemented here (likely AES-CBC)
}

void sk_depot_manifest_destroy(sk_depot_manifest_t* manifest) {
    if (!manifest) return;
    
    if (manifest->files) {
        for (uint32_t i = 0; i < manifest->num_files; ++i) {
            free(manifest->files[i].filename);
            free(manifest->files[i].chunks);
        }
        free(manifest->files);
    }
    
    free(manifest);
}
