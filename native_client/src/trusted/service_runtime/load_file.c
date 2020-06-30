/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <string.h>
#include "native_client/src/trusted/service_runtime/load_file.h"

#include "native_client/src/trusted/desc/nacl_desc_base.h"
#include "native_client/src/trusted/desc/nacl_desc_io.h"
#include "native_client/src/trusted/service_runtime/include/sys/fcntl.h"
#include "native_client/src/trusted/service_runtime/nacl_valgrind_hooks.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"
#include "polarssl_sha/sha256.h"

#define xstr(s) str(s)
#define str(s) #s

void sha256_hash_string(unsigned char* in, unsigned char* out, int in_size, int out_size) {
  int i = 0;

  for (i = 0; i < in_size; i++) {
    sprintf((char*)out + (i*2), "%02x", in[i]);
  }
  out[out_size-1] = 0;
}

NaClErrorCode NaClAppLoadFileFromFilename(struct NaClApp *nap,
                                          const char *filename) {
  struct NaClDesc *nd;
  NaClErrorCode err;
  unsigned char sha256_res[32] = {0};
  unsigned char sha256_res_string[65] = {0};
  int ret;

  NaClFileNameForValgrind(filename);

  ret = sha256_file(filename, sha256_res, 0);
  if (ret == POLARSSL_ERR_SHA256_FILE_IO_ERROR) {
    return LOAD_VALIDATION_FAILED;
  }
  sha256_hash_string(sha256_res, sha256_res_string, 32, 65);
#ifdef NACL_APP_SHA
  if (strncmp((const char *)sha256_res_string, xstr(NACL_APP_SHA), 65) != 0) {
    printf("Actual hash value is: %s\n", sha256_res_string);
    printf("Expected hash value is: %s\n", xstr(NACL_APP_SHA));
    return LOAD_VALIDATION_FAILED;
  }
#endif
  nd = (struct NaClDesc *) NaClDescIoDescOpen(filename, NACL_ABI_O_RDONLY,
                                              0666);
  if (NULL == nd) {
    return LOAD_OPEN_ERROR;
  }

  NaClAppLoadModule(nap, nd, NULL, NULL);
  err = NaClGetLoadStatus(nap);
  NaClDescUnref(nd);
  nd = NULL;

  return err;
}
