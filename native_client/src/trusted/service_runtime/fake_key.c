#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "native_client/src/trusted/service_runtime/fake_key.h"
#include "native_client/src/trusted/service_runtime/libsodium/src/libsodium/include/sodium.h"

static uint8_t * key = NULL;

uint8_t* GetFakeKey(void) {
  if (key == NULL) {
    key = (uint8_t *) malloc(crypto_aead_aes256gcm_KEYBYTES);
    sodium_hex2bin(key, crypto_aead_aes256gcm_KEYBYTES,
                   FAKE_KEY_HEX, strlen(FAKE_KEY_HEX),
                   NULL, NULL, NULL);
  }
  return key;
}

void FakeKeyFinalize(void) {
  free(key);
  key = NULL;
}
