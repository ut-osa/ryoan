#include "native_client/src/trusted/service_runtime/libsodium/src/libsodium/include/sodium/crypto_stream_salsa2012.h"

size_t
crypto_stream_salsa2012_keybytes(void) {
    return crypto_stream_salsa2012_KEYBYTES;
}

size_t
crypto_stream_salsa2012_noncebytes(void) {
    return crypto_stream_salsa2012_NONCEBYTES;
}
