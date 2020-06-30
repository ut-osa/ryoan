#include <string.h>

#include "native_client/src/trusted/service_runtime/libsodium/src/libsodium/include/sodium/crypto_box_curve25519xsalsa20poly1305.h"
#include "native_client/src/trusted/service_runtime/libsodium/src/libsodium/include/sodium/crypto_hash_sha512.h"
#include "native_client/src/trusted/service_runtime/libsodium/src/libsodium/include/sodium/crypto_scalarmult_curve25519.h"
#include "native_client/src/trusted/service_runtime/libsodium/src/libsodium/include/sodium/randombytes.h"
#include "native_client/src/trusted/service_runtime/libsodium/src/libsodium/include/sodium/utils.h"

int crypto_box_curve25519xsalsa20poly1305_seed_keypair(
  unsigned char *pk,
  unsigned char *sk,
  const unsigned char *seed
)
{
  unsigned char hash[64];
  crypto_hash_sha512(hash,seed,32);
  memmove(sk,hash,32);
  sodium_memzero(hash, sizeof hash);
  return crypto_scalarmult_curve25519_base(pk,sk);
}

int crypto_box_curve25519xsalsa20poly1305_keypair(
  unsigned char *pk,
  unsigned char *sk
)
{
  randombytes_buf(sk,32);
  return crypto_scalarmult_curve25519_base(pk,sk);
}
