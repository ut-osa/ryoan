#include "native_client/src/trusted/service_runtime/libsodium/src/libsodium/include/sodium/crypto_stream_xsalsa20.h"

size_t
crypto_stream_xsalsa20_keybytes(void) {
    return crypto_stream_xsalsa20_KEYBYTES;
}

size_t
crypto_stream_xsalsa20_noncebytes(void) {
    return crypto_stream_xsalsa20_NONCEBYTES;
}
