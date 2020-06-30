#include "native_client/src/trusted/service_runtime/libsodium/src/libsodium/include/sodium/crypto_auth_hmacsha256.h"
#include "native_client/src/trusted/service_runtime/libsodium/src/libsodium/include/sodium/crypto_verify_32.h"
#include "native_client/src/trusted/service_runtime/libsodium/src/libsodium/include/sodium/utils.h"

int crypto_auth_hmacsha256_verify(const unsigned char *h,const unsigned char *in,unsigned long long inlen,const unsigned char *k)
{
  unsigned char correct[32];
  crypto_auth_hmacsha256(correct,in,inlen,k);
  return crypto_verify_32(h,correct) | (-(h == correct)) |
         sodium_memcmp(correct,h,32);
}
