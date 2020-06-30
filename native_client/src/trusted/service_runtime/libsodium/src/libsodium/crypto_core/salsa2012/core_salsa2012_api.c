#include "native_client/src/trusted/service_runtime/libsodium/src/libsodium/include/sodium/crypto_core_salsa2012.h"

size_t
crypto_core_salsa2012_outputbytes(void) {
    return crypto_core_salsa2012_OUTPUTBYTES;
}

size_t
crypto_core_salsa2012_inputbytes(void) {
    return crypto_core_salsa2012_INPUTBYTES;
}

size_t
crypto_core_salsa2012_keybytes(void) {
    return crypto_core_salsa2012_KEYBYTES;
}

size_t
crypto_core_salsa2012_constbytes(void) {
    return crypto_core_salsa2012_CONSTBYTES;
}
