
#ifndef randombytes_nativeclient_H
#define randombytes_nativeclient_H

#ifdef __native_client__

# include "native_client/src/trusted/service_runtime/libsodium/src/libsodium/include/sodium/export.h"
# include "native_client/src/trusted/service_runtime/libsodium/src/libsodium/include/sodium/randombytes.h"

# ifdef __cplusplus
extern "C" {
# endif

SODIUM_EXPORT
extern struct randombytes_implementation randombytes_nativeclient_implementation;

# ifdef __cplusplus
}
# endif

#endif

#endif
