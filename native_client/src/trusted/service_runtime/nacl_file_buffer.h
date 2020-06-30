/*
 * Copyright (c) 2016 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_NACL_FILE_BUFFER_H_
#define NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_NACL_FILE_BUFFER_H_

#include <stdint.h>
#include "native_client/src/include/build_config.h"
#include "native_client/src/include/nacl_base.h"
#include "native_client/src/include/portability.h"
#include "native_client/src/trusted/nacl_base/nacl_refcount.h"
#include "native_client/src/trusted/service_runtime/buffer_common.h"

EXTERN_C_BEGIN

struct NaClFileBuffer {
  struct NaClRefCount base NACL_IS_REFCOUNT_SUBCLASS;
  uint32_t size;
  uint32_t len;
  void *base_tbl[BUF_CHUNK_GROUP_SIZE];
  uint8_t *buffer;
  uint32_t pos;
  uint8_t writable;
} NaClFileBuffer;

// with fixed_len is positive, use NaClFileBuffer.buffer, cannot resize;
// otherwise, use NaClFileBuffer.base_tbl, can resize.
int NaClFileBufferCtor_fixed_len(struct NaClFileBuffer *self, int32_t fixed_len) NACL_WUR;
int NaClFileBufferCtor(struct NaClFileBuffer *self) NACL_WUR;

struct NaClFileBuffer *NaClFileBufferRef(struct NaClFileBuffer *fbp);
void NaClFileBufferUnref(struct NaClFileBuffer* fbp);
void NaClFileBufferSafeUnref(struct NaClFileBuffer* fbp);

int32_t NaClFileBufferRead(struct NaClFileBuffer* fbp, void *buf, uint32_t len);
int32_t  NaClFileBufferWrite(struct NaClFileBuffer* fbp, void *buf, uint32_t len);

EXTERN_C_END

#endif
