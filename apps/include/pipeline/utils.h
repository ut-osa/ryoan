#ifndef _UTILS_H_
#define _UTILS_H_
#include <openssl/sha.h>
#include <sgx.h>
#include <unistd.h>
#include "enclaveops.h"

#define __align(v) __attribute__((aligned(v)))
typedef struct hash_t { unsigned char payload[SHA256_DIGEST_LENGTH]; } hash_t;

/* Dummy proofed to init only once, so call all you want */
void init_libssl();
void dump_buffer(unsigned char *, size_t);
int compute_hash(hash_t *hash, const void *data, size_t len);
int _ereport(SGX_report *report, SGX_userdata_t *userdata,
             SGX_targetinfo *targetinfo);
int _egetkey(SGX_key_t *key);
int verify_mac(const SGX_report *report);
#endif
