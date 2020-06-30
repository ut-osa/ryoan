#include "native_client/src/trusted/service_runtime/libsodium/src/libsodium/include/sodium/crypto_auth_hmacsha256.h"

size_t
crypto_auth_hmacsha256_bytes(void) {
    return crypto_auth_hmacsha256_BYTES;
}

size_t
crypto_auth_hmacsha256_keybytes(void) {
    return crypto_auth_hmacsha256_KEYBYTES;
}

size_t
crypto_auth_hmacsha256_statebytes(void) {
    return sizeof(crypto_auth_hmacsha256_state);
}
