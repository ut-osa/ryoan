/*
 * Copyright (c) 2016 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_BUFFER_DEF_H_
#define NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_BUFFER_DEF_H_ 1

#include "native_client/src/include/build_config.h"
#include "native_client/src/include/nacl_base.h"
#include "native_client/src/include/portability.h"

#include <string.h>

EXTERN_C_BEGIN

#define BUF_CHUNK_SIZE_BITS 12
#define BUF_CHUNK_SIZE (1 << BUF_CHUNK_SIZE_BITS) // 4096
#define BUF_CHUNK_GROUP_SIZE_BITS 9
#define BUF_CHUNK_GROUP_SIZE (1 << BUF_CHUNK_GROUP_SIZE_BITS) // 512
#define BUF_GET_UPPER_GROUP(x) (x >> (BUF_CHUNK_SIZE_BITS + BUF_CHUNK_GROUP_SIZE_BITS))
#define BUF_GET_LOWER_GROUP(x) ((x >> BUF_CHUNK_SIZE_BITS) & (BUF_CHUNK_GROUP_SIZE - 1))
#define BUF_POS_IN_CHUNK(x) (x & (BUF_CHUNK_SIZE - 1))
#define BUF_MAX_LEN (1 << (BUF_CHUNK_SIZE_BITS + BUF_CHUNK_GROUP_SIZE_BITS * 2)) // 1 GB

void BaseTblDtor(void **base_tbl);

inline uint8_t *GetCurrentBuffer(void **base_tbl, uint32_t pos) {
  uint32_t i = BUF_GET_UPPER_GROUP(pos);
  uint8_t **t = (uint8_t **) base_tbl[i];
  uint32_t j = BUF_GET_LOWER_GROUP(pos);
  return t[j] + BUF_POS_IN_CHUNK(pos);
}

inline int32_t Grow(void **base_tbl, uint32_t needed, uint32_t *size) {
  while (needed > *size) {
    uint32_t i = BUF_GET_UPPER_GROUP(*size);
    uint8_t **t = (uint8_t **) base_tbl[i];
    uint32_t j = BUF_GET_LOWER_GROUP(*size);
    uint8_t *buf;
    if (t == NULL) {
      t = (uint8_t **)malloc(BUF_CHUNK_GROUP_SIZE * sizeof(void *));
      if (t == NULL)
        return -1;
      base_tbl[i] = t;
      memset(t, 0, BUF_CHUNK_GROUP_SIZE * sizeof(void *));
    }

    buf = t[j];
    if (buf == NULL) {
      buf = malloc(BUF_CHUNK_SIZE);
      if (buf == NULL)
        return -1;
      t[j] = buf;
      memset(buf, 0, BUF_CHUNK_SIZE);
    }
    *size += BUF_CHUNK_SIZE;
  }
  return 0;
}

inline int32_t BufferRead(void **base_tbl, void *buf, uint32_t buf_len, uint32_t *pos) {
  uint32_t bytes = 0;
  while (bytes < buf_len) {
    uint32_t n = BUF_CHUNK_SIZE - BUF_POS_IN_CHUNK(*pos);
    if (n > buf_len - bytes)
      n = buf_len - bytes;
    memcpy(buf, GetCurrentBuffer(base_tbl, *pos), n);
    buf = (uint8_t *)buf + n;
    *pos += n;
    bytes += n;
  }
  return bytes;
}

inline int32_t BufferWrite(void     **base_tbl,
                           void     *buf,
                           uint32_t buf_len,
                           uint32_t *tbl_len,
                           uint32_t *pos) {
  uint32_t bytes = 0;
  while (bytes < buf_len) {
    uint32_t n = BUF_CHUNK_SIZE - BUF_POS_IN_CHUNK(*pos);
    if (n > buf_len - bytes)
      n = buf_len - bytes;
    memcpy(GetCurrentBuffer(base_tbl, *pos), buf, n);
    buf = (uint8_t *)buf + n;
    *pos += n;
    bytes += n;
    if (*tbl_len < *pos)
      *tbl_len = *pos;
  }
  return bytes;
}

EXTERN_C_END

#endif
