
#ifndef sodium_core_H
#define sodium_core_H

#include "native_client/src/trusted/service_runtime/libsodium/src/libsodium/include/sodium/export.h"

#ifdef __cplusplus
extern "C" {
#endif

SODIUM_EXPORT
int sodium_init(void)
            __attribute__ ((warn_unused_result));

#ifdef __cplusplus
}
#endif

#endif
