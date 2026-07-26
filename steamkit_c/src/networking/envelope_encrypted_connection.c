#include "steamkit/networking/envelope_encrypted_connection.h"
#include "steamkit/networking/connection.h"
#include "steamkit/networking/tcp_connection.h"
#include "steamkit/networking/websocket_connection.h"
#include "steamkit/utils/crypto_helper.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#define ENVELOPE_HEADER_SIZE 4

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

uint8_t* sk_envelope_encrypted_connection_encrypt(sk_envelope_encrypted_connection_t* enc, const uint8_t* data, size_t len, size_t* out_len) {
    if (!enc || !enc->key || enc->key_len == 0) {
        if (out_len) *out_len = 0;
        return NULL;
    }
    uint8_t* hmac = sk_crypto_hmac_sha1(enc->key, enc->key_len, data, len, out_len);
    if (!hmac) return NULL;

    size_t total = ENVELOPE_HEADER_SIZE + *out_len + len;
    uint8_t* result = (uint8_t*)malloc(total);
    if (!result) {
        free(hmac);
        if (out_len) *out_len = 0;
        return NULL;
    }

    uint32_t hmac_len = (uint32_t)*out_len;
    result[0] = hmac_len & 0xFF;
    result[1] = (hmac_len >> 8) & 0xFF;
    result[2] = (hmac_len >> 16) & 0xFF;
    result[3] = (hmac_len >> 24) & 0xFF;
    memcpy(result + ENVELOPE_HEADER_SIZE, hmac, hmac_len);
    memcpy(result + ENVELOPE_HEADER_SIZE + hmac_len, data, len);

    free(hmac);
    *out_len = total;
    return result;
}

uint8_t* sk_envelope_encrypted_connection_decrypt(sk_envelope_encrypted_connection_t* enc, const uint8_t* data, size_t len, size_t* out_len) {
    if (!enc || !enc->key || enc->key_len == 0 || len < ENVELOPE_HEADER_SIZE) {
        if (out_len) *out_len = 0;
        return NULL;
    }

    uint32_t hmac_len = (uint32_t)data[0] |
                        ((uint32_t)data[1] << 8) |
                        ((uint32_t)data[2] << 16) |
                        ((uint32_t)data[3] << 24);

    if (len < ENVELOPE_HEADER_SIZE + hmac_len) {
        if (out_len) *out_len = 0;
        return NULL;
    }

    const uint8_t* payload = data + ENVELOPE_HEADER_SIZE + hmac_len;
    size_t payload_len = len - ENVELOPE_HEADER_SIZE - hmac_len;

    const uint8_t* stored_hmac = data + ENVELOPE_HEADER_SIZE;
    size_t computed_hmac_len = 0;
    uint8_t* computed_hmac = sk_crypto_hmac_sha1(enc->key, enc->key_len, payload, payload_len, &computed_hmac_len);
    if (!computed_hmac) {
        if (out_len) *out_len = 0;
        return NULL;
    }

    if (computed_hmac_len != hmac_len || memcmp(computed_hmac, stored_hmac, hmac_len) != 0) {
        free(computed_hmac);
        if (out_len) *out_len = 0;
        return NULL;
    }

    free(computed_hmac);

    uint8_t* result = (uint8_t*)malloc(payload_len);
    if (!result) {
        if (out_len) *out_len = 0;
        return NULL;
    }
    memcpy(result, payload, payload_len);
    *out_len = payload_len;
    return result;
}

void sk_envelope_encrypted_connection_send(sk_envelope_encrypted_connection_t* enc, const uint8_t* data, size_t len) {
    if (!enc || !enc->inner) return;

    size_t encrypted_len = 0;
    uint8_t* encrypted = sk_envelope_encrypted_connection_encrypt(enc, data, len, &encrypted_len);
    if (encrypted && encrypted_len > 0) {
        sk_protocol_type_t proto = sk_connection_protocol_type(enc->inner);
        if (proto == SK_PROTOCOL_TYPE_TCP) {
            sk_tcp_connection_send((sk_tcp_connection_t*)enc->inner->impl, encrypted, encrypted_len);
        } else {
            sk_connection_send(enc->inner, encrypted, encrypted_len);
        }
        free(encrypted);
    }
}

ssize_t sk_envelope_encrypted_connection_recv(sk_envelope_encrypted_connection_t* enc, uint8_t* buf, size_t buf_len, int timeout_ms) {
    if (!enc || !enc->inner || !buf) return -1;

    ssize_t n = -1;
    sk_protocol_type_t proto = sk_connection_protocol_type(enc->inner);
    if (proto == SK_PROTOCOL_TYPE_TCP) {
        n = sk_tcp_connection_recv((sk_tcp_connection_t*)enc->inner->impl, buf, buf_len, timeout_ms);
    } else {
        n = sk_connection_recv(enc->inner, buf, buf_len, timeout_ms);
    }
    if (n <= 0) return n;

    size_t decrypted_len = 0;
    uint8_t* decrypted = sk_envelope_encrypted_connection_decrypt(enc, buf, (size_t)n, &decrypted_len);
    if (!decrypted) return -1;

    if (decrypted_len > buf_len) {
        free(decrypted);
        return -1;
    }

    memcpy(buf, decrypted, decrypted_len);
    free(decrypted);
    return (ssize_t)decrypted_len;
}

void sk_envelope_encrypted_connection_destroy(sk_envelope_encrypted_connection_t* enc) {
    if (!enc) return;
    free(enc->key);
    free(enc);
}
