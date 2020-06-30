#include "native_client/src/trusted/service_runtime/libsodium/src/libsodium/include/sodium/crypto_stream_aes128ctr.h"

size_t
crypto_stream_aes128ctr_keybytes(void) {
    return crypto_stream_aes128ctr_KEYBYTES;
}

size_t
crypto_stream_aes128ctr_noncebytes(void) {
    return crypto_stream_aes128ctr_NONCEBYTES;
}

size_t
crypto_stream_aes128ctr_beforenmbytes(void) {
    return crypto_stream_aes128ctr_BEFORENMBYTES;
}
