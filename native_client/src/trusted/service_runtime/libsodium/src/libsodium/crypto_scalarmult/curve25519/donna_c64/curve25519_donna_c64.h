#ifndef curve25519_donna_c64_H
#define curve25519_donna_c64_H

#include "native_client/src/trusted/service_runtime/libsodium/src/libsodium/include/sodium/crypto_scalarmult_curve25519.h"
#include "../scalarmult_curve25519.h"

extern struct crypto_scalarmult_curve25519_implementation
    crypto_scalarmult_curve25519_donna_c64_implementation;

#endif
