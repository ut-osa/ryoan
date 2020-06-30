
#ifndef randombytes_sysrandom_H
#define randombytes_sysrandom_H

#include "native_client/src/trusted/service_runtime/libsodium/src/libsodium/include/sodium/export.h"
#include "native_client/src/trusted/service_runtime/libsodium/src/libsodium/include/sodium/randombytes.h"

#ifdef __cplusplus
extern "C" {
#endif

SODIUM_EXPORT
extern struct randombytes_implementation randombytes_sysrandom_implementation;

#ifdef __cplusplus
}
#endif

#endif
