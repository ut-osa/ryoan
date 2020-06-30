#ifndef poly1305_sse2_H
#define poly1305_sse2_H

#include <stddef.h>

#include "native_client/src/trusted/service_runtime/libsodium/src/libsodium/include/sodium/crypto_onetimeauth_poly1305.h"
#include "../onetimeauth_poly1305.h"

extern struct crypto_onetimeauth_poly1305_implementation
    crypto_onetimeauth_poly1305_sse2_implementation;

#endif /* poly1305_sse2_H */