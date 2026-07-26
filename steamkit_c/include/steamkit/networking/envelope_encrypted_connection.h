#ifndef STEAMKIT_NETWORKING_ENVELOPE_ENCRYPTED_CONNECTION_H
#define STEAMKIT_NETWORKING_ENVELOPE_ENCRYPTED_CONNECTION_H

#include "steamkit/networking/connection.h"

#ifdef __cplusplus
extern "C" {
#endif

// Encrypted connection wrapper - mirrors C# EnvelopeEncryptedConnection
typedef struct sk_envelope_encrypted_connection sk_envelope_encrypted_connection_t;

// Creates an encrypted connection wrapper
sk_envelope_encrypted_connection_t* sk_envelope_encrypted_connection_create(sk_connection_t* inner);

// Sets the HMAC key for encryption
void sk_envelope_encrypted_connection_set_key(sk_envelope_encrypted_connection_t* enc, const uint8_t* key, size_t key_len);

// Encrypts data using the envelope key
uint8_t* sk_envelope_encrypted_connection_encrypt(sk_envelope_encrypted_connection_t* enc, const uint8_t* data, size_t len, size_t* out_len);

// Decrypts data using the envelope key
uint8_t* sk_envelope_encrypted_connection_decrypt(sk_envelope_encrypted_connection_t* enc, const uint8_t* data, size_t len, size_t* out_len);

// Sends encrypted data through the inner connection
void sk_envelope_encrypted_connection_send(sk_envelope_encrypted_connection_t* enc, const uint8_t* data, size_t len);

// Receives and decrypts data from the inner connection
ssize_t sk_envelope_encrypted_connection_recv(sk_envelope_encrypted_connection_t* enc, uint8_t* buf, size_t buf_len, int timeout_ms);

// Destroys an encrypted connection wrapper
void sk_envelope_encrypted_connection_destroy(sk_envelope_encrypted_connection_t* enc);

#ifdef __cplusplus
}
#endif

#endif // STEAMKIT_NETWORKING_ENVELOPE_ENCRYPTED_CONNECTION_H
