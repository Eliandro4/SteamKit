#include "steamkit/networking/envelope_encrypted_connection.h"
#include <stdlib.h>
#include <string.h>

typedef struct sk_envelope_encrypted_connection {
    sk_connection_t base;
    sk_connection_t* inner;
    uint8_t* key;
    size_t key_len;
} sk_envelope_encrypted_connection_t;

sk_envelope_encrypted_connection_t* sk_envelope_encrypted_connection_create(sk_connection_t* inner) {
    sk_envelope_encrypted_connection_t* enc = (sk_envelope_encrypted_connection_t*)calloc(1, sizeof(sk_envelope_encrypted_connection_t));
    if (enc) {
        enc->base.impl = enc;
        enc->inner = inner;
    }
    return enc;
}

void sk_envelope_encrypted_connection_set_key(sk_envelope_encrypted_connection_t* enc, const uint8_t* key, size_t key_len) {
    if (!enc) return;
    free(enc->key);
    enc->key_len = key_len;
    enc->key = (uint8_t*)malloc(key_len);
    if (enc->key && key) {
        memcpy(enc->key, key, key_len);
    }
}

void sk_envelope_encrypted_connection_destroy(sk_envelope_encrypted_connection_t* enc) {
    if (!enc) return;
    free(enc->key);
    free(enc);
}
