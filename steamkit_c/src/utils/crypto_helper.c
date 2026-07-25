#include "steamkit/utils/crypto_helper.h"
#include <stdlib.h>
#include <string.h>

#ifdef SK_ENABLE_OPENSSL
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/aes.h>
#include <openssl/rsa.h>

uint8_t* sk_crypto_symmetric_decrypt(const uint8_t* input, size_t input_len,
                                     const uint8_t* key, size_t key_len,
                                     size_t* out_len) {
    (void)input;
    (void)input_len;
    (void)key;
    (void)key_len;
    if (out_len) *out_len = 0;
    return NULL;
}

uint8_t* sk_crypto_symmetric_encrypt(const uint8_t* input, size_t input_len,
                                     const uint8_t* key, size_t key_len,
                                     size_t* out_len) {
    (void)input;
    (void)input_len;
    (void)key;
    (void)key_len;
    if (out_len) *out_len = 0;
    return NULL;
}

uint8_t* sk_crypto_aes_ecb_encrypt(const uint8_t* input, size_t input_len,
                                    const uint8_t* key, size_t key_len,
                                    size_t* out_len) {
    if (!input || !key || key_len != 32 || input_len % 16 != 0) {
        if (out_len) *out_len = 0;
        return NULL;
    }
    
    uint8_t* output = malloc(input_len);
    if (!output) {
        if (out_len) *out_len = 0;
        return NULL;
    }
    
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        free(output);
        if (out_len) *out_len = 0;
        return NULL;
    }
    
    EVP_EncryptInit_ex(ctx, EVP_aes_256_ecb(), NULL, key, NULL);
    EVP_CIPHER_CTX_set_padding(ctx, 0);
    
    int len = 0;
    EVP_EncryptUpdate(ctx, output, &len, input, (int)input_len);
    
    EVP_CIPHER_CTX_free(ctx);
    
    if (out_len) *out_len = (size_t)len;
    return output;
}

uint8_t* sk_crypto_hmac_sha1(const uint8_t* key, size_t key_len,
                             const uint8_t* data, size_t data_len,
                             size_t* out_len) {
    if (!key || !data) {
        if (out_len) *out_len = 0;
        return NULL;
    }
    
    uint8_t* result = malloc(EVP_MAX_MD_SIZE);
    if (!result) {
        if (out_len) *out_len = 0;
        return NULL;
    }
    
    unsigned int md_len = 0;
    HMAC(EVP_sha1(), key, (int)key_len, data, data_len, result, &md_len);
    
    if (out_len) *out_len = md_len;
    return result;
}

uint8_t* sk_crypto_sha1(const uint8_t* data, size_t data_len, size_t* out_len) {
    if (!data) {
        if (out_len) *out_len = 0;
        return NULL;
    }
    
    uint8_t* result = malloc(EVP_MAX_MD_SIZE);
    if (!result) {
        if (out_len) *out_len = 0;
        return NULL;
    }
    
    unsigned int md_len = 0;
    EVP_Digest(data, data_len, result, &md_len, EVP_sha1(), NULL);
    
    if (out_len) *out_len = md_len;
    return result;
}

uint8_t* sk_crypto_md5(const uint8_t* data, size_t data_len, size_t* out_len) {
    if (!data) {
        if (out_len) *out_len = 0;
        return NULL;
    }
    
    uint8_t* result = malloc(EVP_MAX_MD_SIZE);
    if (!result) {
        if (out_len) *out_len = 0;
        return NULL;
    }
    
    unsigned int md_len = 0;
    EVP_Digest(data, data_len, result, &md_len, EVP_md5(), NULL);
    
    if (out_len) *out_len = md_len;
    return result;
}

uint8_t* sk_crypto_rsa_encrypt(const uint8_t* input, size_t input_len,
                                const uint8_t* public_key, size_t key_len,
                                size_t* out_len) {
    (void)input;
    (void)input_len;
    (void)public_key;
    (void)key_len;
    if (out_len) *out_len = 0;
    return NULL;
}

bool sk_crypto_is_available(void) {
    return true;
}

#else

bool sk_crypto_is_available(void) {
    return false;
}

uint8_t* sk_crypto_symmetric_decrypt(const uint8_t* input, size_t input_len,
                                     const uint8_t* key, size_t key_len,
                                     size_t* out_len) {
    (void)input; (void)input_len; (void)key; (void)key_len;
    if (out_len) *out_len = 0;
    return NULL;
}

uint8_t* sk_crypto_symmetric_encrypt(const uint8_t* input, size_t input_len,
                                     const uint8_t* key, size_t key_len,
                                     size_t* out_len) {
    (void)input; (void)input_len; (void)key; (void)key_len;
    if (out_len) *out_len = 0;
    return NULL;
}

uint8_t* sk_crypto_aes_ecb_encrypt(const uint8_t* input, size_t input_len,
                                    const uint8_t* key, size_t key_len,
                                    size_t* out_len) {
    (void)input; (void)input_len; (void)key; (void)key_len;
    if (out_len) *out_len = 0;
    return NULL;
}

uint8_t* sk_crypto_hmac_sha1(const uint8_t* key, size_t key_len,
                             const uint8_t* data, size_t data_len,
                             size_t* out_len) {
    (void)key; (void)key_len; (void)data; (void)data_len;
    if (out_len) *out_len = 0;
    return NULL;
}

uint8_t* sk_crypto_sha1(const uint8_t* data, size_t data_len, size_t* out_len) {
    (void)data; (void)data_len;
    if (out_len) *out_len = 0;
    return NULL;
}

uint8_t* sk_crypto_md5(const uint8_t* data, size_t data_len, size_t* out_len) {
    (void)data; (void)data_len;
    if (out_len) *out_len = 0;
    return NULL;
}

uint8_t* sk_crypto_rsa_encrypt(const uint8_t* input, size_t input_len,
                                const uint8_t* public_key, size_t key_len,
                                size_t* out_len) {
    (void)input; (void)input_len; (void)public_key; (void)key_len;
    if (out_len) *out_len = 0;
    return NULL;
}

#endif
