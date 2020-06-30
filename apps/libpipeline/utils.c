#include <assert.h>
#include <fcntl.h>
#include <openssl/cmac.h>
#include <openssl/conf.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <string.h>
#include <unistd.h>

#include "pipeline/utils.h"

#define HOST_ENTROPY "/root/host_entropy"

typedef struct mac_t {
  unsigned char digest[EVP_MAX_MD_SIZE];
  size_t len;
} mac_t;

const char *__error_string;
void init_libssl() {
  static int inited;
  if (inited) return;
  inited = 1;
  ERR_load_crypto_strings();
  OpenSSL_add_all_algorithms();
  OPENSSL_config(NULL);

  RAND_seed("some fake entropy", 17);
}

void dump_buffer(unsigned char *buffer, size_t size) {
  int i;
  for (i = 0; i < 32; ++i) {
    fprintf(stderr, "%hhx", buffer[i]);
  }
  fprintf(stderr, "\n");
  fflush(stderr);
}

int compute_hash(hash_t *hash, const void *data, size_t len) {
  SHA256(data, len, hash->payload);
  return 0;
}

int _ereport(SGX_report *report, SGX_userdata_t *userdata,
             SGX_targetinfo *targetinfo) {
#ifdef NOT_SGX
  report->userdata = *userdata;
#else
  SGX_report _report __align(512);
  ereport(targetinfo, userdata, &_report);
  *report = _report;
#endif
  return 0;
}

int _egetkey(SGX_key_t *key) {
  int ret;
#ifndef NOT_SGX
  SGX_keyrequest request __align(128);
  request.keyname = REPORT_KEY;
  ret = egetkey(&request, key);
#else
  memset(key, 0, sizeof(SGX_key_t));
  ret = 0;
#endif
  return ret;
}

static size_t cmac(cmac_t *result, SGX_key_t *key, const void *input,
                   size_t len) {
  size_t size;
  CMAC_CTX *ctx = CMAC_CTX_new();
  CMAC_Init(ctx, key, sizeof(SGX_key_t), EVP_aes_128_cbc(), NULL);
  CMAC_Update(ctx, input, len);
  CMAC_Final(ctx, (unsigned char *)result, &size);
  CMAC_CTX_free(ctx);
  return size;
}

static void dump_cmac_t(const cmac_t *mac) {
  int i;
  for (i = 0; i < sizeof(cmac_t); ++i) {
    printf("%hhx", mac->payload[i]);
  }
}

int verify_mac(const SGX_report *report) {
  cmac_t result;
  size_t mac_size;
  SGX_key_t key __align(16);

  _egetkey(&key);
  mac_size = cmac(&result, &key, report, offsetof(SGX_report, mac));
  if (memcmp(&result, &report->mac, mac_size) == 0) {
    return 0;
  } else {
    printf("\n\nGiven mac:");
    dump_cmac_t(&report->mac);
    printf("\nComputed mac:");
    dump_cmac_t(&result);
    printf("\n");
    return -1;
  }
}

void dump_key_t(const SGX_key_t *key) {
  int i;
  for (i = 0; i < sizeof(SGX_key_t); ++i) {
    printf("%hhx", key->payload[i]);
  }
}
