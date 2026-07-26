#ifndef STEAMKIT_UTILS_CRYPTO_HELPER_H
#define STEAMKIT_UTILS_CRYPTO_HELPER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Symmetric decryption using AES/CBC/PKCS7 with ECB prepended IV
// Returns a newly allocated buffer that must be freed by the caller
uint8_t* sk_crypto_symmetric_decrypt(const uint8_t* input, size_t input_len, 
                                     const uint8_t* key, size_t key_len,
                                     size_t* out_len);

// Symmetric encryption using AES/CBC/PKCS7
uint8_t* sk_crypto_symmetric_encrypt(const uint8_t* input, size_t input_len,
                                     const uint8_t* key, size_t key_len,
                                     size_t* out_len);

// AES encryption using ECB mode (for IV encryption)
uint8_t* sk_crypto_aes_ecb_encrypt(const uint8_t* input, size_t input_len,
                                    const uint8_t* key, size_t key_len,
                                    size_t* out_len);

// HMAC-SHA1
uint8_t* sk_crypto_hmac_sha1(const uint8_t* key, size_t key_len,
                             const uint8_t* data, size_t data_len,
                             size_t* out_len);

// SHA1 hash
uint8_t* sk_crypto_sha1(const uint8_t* data, size_t data_len, size_t* out_len);

// MD5 hash
uint8_t* sk_crypto_md5(const uint8_t* data, size_t data_len, size_t* out_len);

// RSA encryption
// public_key should be a null-terminated string in format "modulus_hex|exponent_hex"
uint8_t* sk_crypto_rsa_encrypt(const uint8_t* input, size_t input_len,
                               const uint8_t* public_key, size_t key_len,
                               size_t* out_len);

// Base64 encode
char* sk_crypto_base64_encode(const uint8_t* input, size_t input_len);

// Check if OpenSSL is available
bool sk_crypto_is_available(void);

// AES/CBC decrypt a null-terminated encrypted string (base64-like) into a newly allocated string
char* sk_crypto_aes_cbc_decrypt_string(const uint8_t* key, const char* encrypted);

#ifdef __cplusplus
}
#endif

#endif // STEAMKIT_UTILS_CRYPTO_HELPER_H
