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

// Destroys an encrypted connection wrapper
void sk_envelope_encrypted_connection_destroy(sk_envelope_encrypted_connection_t* enc);

#ifdef __cplusplus
}
#endif

#endif // STEAMKIT_NETWORKING_ENVELOPE_ENCRYPTED_CONNECTION_H
