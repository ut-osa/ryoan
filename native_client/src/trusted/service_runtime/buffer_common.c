/*
 * Copyright (c) 2016 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/trusted/service_runtime/buffer_common.h"
#include "native_client/src/shared/platform/nacl_log.h"

#include <stdlib.h>

void BaseTblDtor(void **base_tbl) {
  for (int i = 0; i < BUF_CHUNK_GROUP_SIZE; i++) {
    uint8_t **t = (uint8_t **) base_tbl[i];
    if (t == NULL) {
      break;
    }
    for (int j = 0; j < BUF_CHUNK_GROUP_SIZE; j++) {
      uint8_t *buf = t[j];
      if (buf == NULL) {
        break;
      }
      free(buf);
    }
    free(t);
  }
}

