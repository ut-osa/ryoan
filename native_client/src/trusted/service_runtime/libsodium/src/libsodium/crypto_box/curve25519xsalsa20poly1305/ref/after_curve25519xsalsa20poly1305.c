#include "native_client/src/trusted/service_runtime/libsodium/src/libsodium/include/sodium/crypto_box_curve25519xsalsa20poly1305.h"
#include "native_client/src/trusted/service_runtime/libsodium/src/libsodium/include/sodium/crypto_secretbox_xsalsa20poly1305.h"

int crypto_box_curve25519xsalsa20poly1305_afternm(
  unsigned char *c,
  const unsigned char *m,unsigned long long mlen,
  const unsigned char *n,
  const unsigned char *k
)
{
  return crypto_secretbox_xsalsa20poly1305(c,m,mlen,n,k);
}

int crypto_box_curve25519xsalsa20poly1305_open_afternm(
  unsigned char *m,
  const unsigned char *c,unsigned long long clen,
  const unsigned char *n,
  const unsigned char *k
)
{
  return crypto_secretbox_xsalsa20poly1305_open(m,c,clen,n,k);
}
