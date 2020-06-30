
#include "native_client/src/trusted/service_runtime/libsodium/src/libsodium/include/sodium/core.h"
#include "native_client/src/trusted/service_runtime/libsodium/src/libsodium/include/sodium/crypto_generichash.h"
#include "native_client/src/trusted/service_runtime/libsodium/src/libsodium/include/sodium/crypto_onetimeauth.h"
#include "native_client/src/trusted/service_runtime/libsodium/src/libsodium/include/sodium/crypto_pwhash_argon2i.h"
#include "native_client/src/trusted/service_runtime/libsodium/src/libsodium/include/sodium/crypto_scalarmult.h"
#include "native_client/src/trusted/service_runtime/libsodium/src/libsodium/include/sodium/crypto_stream_chacha20.h"
#include "native_client/src/trusted/service_runtime/libsodium/src/libsodium/include/sodium/randombytes.h"
#include "native_client/src/trusted/service_runtime/libsodium/src/libsodium/include/sodium/runtime.h"
#include "native_client/src/trusted/service_runtime/libsodium/src/libsodium/include/sodium/utils.h"

#if !defined(_MSC_VER) && 0
# warning This is unstable, untested, development code.
# warning It might not compile. It might not work as expected.
# warning It might be totally insecure.
# warning Do not use this in production.
# warning Use releases available at https://download.libsodium.org/libsodium/releases/ instead.
# warning Alternatively, use the "stable" branch in the git repository.
#endif

static int initialized;

int
sodium_init(void)
{
    if (initialized != 0) {
        return 1;
    }
    _sodium_runtime_get_cpu_features();
    randombytes_stir();
    _sodium_alloc_init();
    _crypto_pwhash_argon2i_pick_best_implementation();
    _crypto_generichash_blake2b_pick_best_implementation();
    _crypto_onetimeauth_poly1305_pick_best_implementation();
    _crypto_scalarmult_curve25519_pick_best_implementation();
    _crypto_stream_chacha20_pick_best_implementation();
    initialized = 1;

    return 0;
}
