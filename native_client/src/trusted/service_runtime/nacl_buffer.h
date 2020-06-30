/*
 * Copyright (c) 2016 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_NACL_BUFFER_H_
#define NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_NACL_BUFFER_H_

#include <stdint.h>
#include <stdbool.h>
#include "native_client/src/include/build_config.h"
#include "native_client/src/include/nacl_base.h"
#include "native_client/src/include/portability.h"
#include "native_client/src/trusted/nacl_base/nacl_refcount.h"
#include "native_client/src/trusted/service_runtime/buffer_common.h"

EXTERN_C_BEGIN

struct NaClBuffer {
  struct NaClRefCount base NACL_IS_REFCOUNT_SUBCLASS;
  uint32_t size;
  uint32_t len;
  uint32_t pos;
  uint32_t num_item_needed;
  uint32_t num_item_got;
  bool need_to_get_total_count;
  void* base_tbl[BUF_CHUNK_GROUP_SIZE];
} NaClBuffer;

int NaClBufferCtor(struct NaClBuffer *self) NACL_WUR;
struct NaClBuffer *NaClBufferRef(struct NaClBuffer *bp);
void NaClBufferUnref(struct NaClBuffer* bp);
void NaClBufferSafeUnref(struct NaClBuffer* bp);
int32_t NaClBufferRead(struct NaClBuffer *bp, void *buf, uint32_t len);
int32_t NaClBufferWrite(struct NaClBuffer *bp, void *buf, uint32_t len);

EXTERN_C_END

#endif
