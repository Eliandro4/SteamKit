#include "steamkit/utils/crypto_helper.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef SK_ENABLE_OPENSSL
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/aes.h>
#include <openssl/rsa.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>

static uint8_t* sk_crypto_aes_cbc_decrypt_internal(const uint8_t* input, size_t input_len,
                                                    const uint8_t* key, size_t key_len,
                                                    const uint8_t* iv, size_t iv_len,
                                                    size_t* out_len) {
    if (!input || !key || key_len != 32 || !iv || iv_len != 16 || input_len == 0) {
        if (out_len) *out_len = 0;
        return NULL;
    }

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        if (out_len) *out_len = 0;
        return NULL;
    }

    uint8_t* output = malloc(input_len + 16);
    if (!output) {
        EVP_CIPHER_CTX_free(ctx);
        if (out_len) *out_len = 0;
        return NULL;
    }

    EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv);
    EVP_CIPHER_CTX_set_padding(ctx, 1);

    int len = 0;
    int rc = EVP_DecryptUpdate(ctx, output, &len, input, (int)input_len);
    if (rc != 1) {
        free(output);
        EVP_CIPHER_CTX_free(ctx);
        if (out_len) *out_len = 0;
        return NULL;
    }

    int final_len = 0;
    rc = EVP_DecryptFinal_ex(ctx, output + len, &final_len);
    if (rc != 1) {
        free(output);
        EVP_CIPHER_CTX_free(ctx);
        if (out_len) *out_len = 0;
        return NULL;
    }

    EVP_CIPHER_CTX_free(ctx);
    if (out_len) *out_len = (size_t)(len + final_len);
    return output;
}

uint8_t* sk_crypto_symmetric_decrypt(const uint8_t* input, size_t input_len,
                                     const uint8_t* key, size_t key_len,
                                     size_t* out_len) {
    if (!input || input_len < 16 || !key || key_len != 32) {
        if (out_len) *out_len = 0;
        return NULL;
    }
    uint8_t iv[16];
    memcpy(iv, input, 16);
    return sk_crypto_aes_cbc_decrypt_internal(input + 16, input_len - 16, key, key_len, iv, 16, out_len);
}

uint8_t* sk_crypto_symmetric_encrypt(const uint8_t* input, size_t input_len,
                                     const uint8_t* key, size_t key_len,
                                     size_t* out_len) {
    if (!input || !key || key_len != 32 || input_len == 0) {
        if (out_len) *out_len = 0;
        return NULL;
    }

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        if (out_len) *out_len = 0;
        return NULL;
    }

    uint8_t iv[16] = {0};
    uint8_t* output = malloc(input_len + 16);
    if (!output) {
        EVP_CIPHER_CTX_free(ctx);
        if (out_len) *out_len = 0;
        return NULL;
    }

    EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv);
    EVP_CIPHER_CTX_set_padding(ctx, 1);

    int len = 0;
    EVP_EncryptUpdate(ctx, output, &len, input, (int)input_len);

    int final_len = 0;
    EVP_EncryptFinal_ex(ctx, output + len, &final_len);

    EVP_CIPHER_CTX_free(ctx);

    if (out_len) *out_len = (size_t)(len + final_len);
    return output;
}

uint8_t* sk_crypto_aes_ecb_encrypt(const uint8_t* input, size_t input_len,
                                    const uint8_t* key, size_t key_len,
                                    size_t* out_len) {
    if (!input || !key || key_len != 32 || input_len % 16 != 0) {
        if (out_len) *out_len = 0;
        return NULL;
    }

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        if (out_len) *out_len = 0;
        return NULL;
    }

    uint8_t* output = malloc(input_len);
    if (!output) {
        EVP_CIPHER_CTX_free(ctx);
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
                               const uint8_t* public_key_hex, size_t key_len,
                               size_t* out_len) {
    if (!input || input_len == 0 || !public_key_hex || key_len == 0) {
        if (out_len) *out_len = 0;
        return NULL;
    }

    const char* mod_hex_str = (const char*)public_key_hex;
    const char* exp_hex = strchr(mod_hex_str, '|');
    if (!exp_hex) {
        if (out_len) *out_len = 0;
        return NULL;
    }
    size_t mod_hex_len = (size_t)(exp_hex - mod_hex_str);
    exp_hex++;

    char* mod_hex = (char*)malloc(mod_hex_len + 1);
    if (!mod_hex) {
        if (out_len) *out_len = 0;
        return NULL;
    }
    memcpy(mod_hex, mod_hex_str, mod_hex_len);
    mod_hex[mod_hex_len] = '\0';

    BIGNUM* bn_mod = NULL;
    BIGNUM* bn_exp = NULL;
    if (BN_hex2bn(&bn_mod, mod_hex) == 0 || BN_hex2bn(&bn_exp, exp_hex) == 0) {
        free(mod_hex);
        BN_free(bn_mod);
        BN_free(bn_exp);
        if (out_len) *out_len = 0;
        return NULL;
    }
    free(mod_hex);

    RSA* rsa = RSA_new();
    if (!rsa || !RSA_set0_key(rsa, bn_mod, bn_exp, NULL)) {
        RSA_free(rsa);
        BN_free(bn_mod);
        BN_free(bn_exp);
        if (out_len) *out_len = 0;
        return NULL;
    }

    int key_size = RSA_size(rsa);
    uint8_t* output = (uint8_t*)malloc((size_t)key_size);
    if (!output) {
        RSA_free(rsa);
        if (out_len) *out_len = 0;
        return NULL;
    }

    int result = RSA_public_encrypt((int)input_len, input, output, rsa, RSA_PKCS1_PADDING);
    RSA_free(rsa);
    if (result <= 0) {
        free(output);
        if (out_len) *out_len = 0;
        return NULL;
    }

    if (out_len) *out_len = (size_t)result;
    return output;
}

bool sk_crypto_is_available(void) {
    return true;
}

char* sk_crypto_base64_encode(const uint8_t* input, size_t input_len) {
    if (!input || input_len == 0) return NULL;
    size_t enc_len = 4 * ((input_len + 2) / 3);
    char* output = (char*)malloc(enc_len + 1);
    if (!output) return NULL;
    int written = EVP_EncodeBlock((unsigned char*)output, input, (int)input_len);
    if (written < 0 || (size_t)written != enc_len) {
        free(output);
        return NULL;
    }
    output[enc_len] = '\0';
    return output;
}

char* sk_crypto_aes_cbc_decrypt_string(const uint8_t* key, const char* encrypted) {
    if (!key || !encrypted) return NULL;

    size_t enc_len = strlen(encrypted);
    if (enc_len == 0 || enc_len % 2 != 0) return NULL;

    size_t bin_len = enc_len / 2;
    uint8_t* bin = malloc(bin_len);
    if (!bin) return NULL;

    for (size_t i = 0; i < bin_len; ++i) {
        unsigned int byte = 0;
        if (sscanf(encrypted + i * 2, "%2x", &byte) != 1) {
            free(bin);
            return NULL;
        }
        bin[i] = (uint8_t)byte;
    }

    uint8_t iv[16] = {0};
    size_t out_len = 0;
    uint8_t* decrypted = sk_crypto_aes_cbc_decrypt_internal(bin + 16, bin_len - 16, key, 32, iv, 16, &out_len);
    free(bin);
    if (!decrypted) return NULL;

    char* result = malloc(out_len + 1);
    if (!result) {
        free(decrypted);
        return NULL;
    }
    memcpy(result, decrypted, out_len);
    result[out_len] = '\0';
    free(decrypted);
    return result;
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

char* sk_crypto_base64_encode(const uint8_t* input, size_t input_len) {
    (void)input; (void)input_len;
    return NULL;
}

char* sk_crypto_aes_cbc_decrypt_string(const uint8_t* key, const char* encrypted) {
    (void)key; (void)encrypted;
    return NULL;
}

#endif
