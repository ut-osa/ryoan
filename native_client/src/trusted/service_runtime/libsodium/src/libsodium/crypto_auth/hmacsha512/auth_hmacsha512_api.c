#include "native_client/src/trusted/service_runtime/libsodium/src/libsodium/include/sodium/crypto_auth_hmacsha512.h"

size_t
crypto_auth_hmacsha512_bytes(void) {
    return crypto_auth_hmacsha512_BYTES;
}

size_t
crypto_auth_hmacsha512_keybytes(void) {
    return crypto_auth_hmacsha512_KEYBYTES;
}

size_t
crypto_auth_hmacsha512_statebytes(void) {
    return sizeof(crypto_auth_hmacsha512_state);
}
