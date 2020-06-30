#include "native_client/src/trusted/service_runtime/libsodium/src/libsodium/include/sodium/crypto_stream_salsa208.h"

size_t
crypto_stream_salsa208_keybytes(void) {
    return crypto_stream_salsa208_KEYBYTES;
}

size_t
crypto_stream_salsa208_noncebytes(void) {
    return crypto_stream_salsa208_NONCEBYTES;
}
