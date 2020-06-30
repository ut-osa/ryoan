/*
 * Copyright (c) 2016 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_DATA_FORMAT_H_
#define NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_DATA_FORMAT_H_

#include <stdint.h>
#include "native_client/src/include/nacl_base.h"

#define FRAME_SIZE 255

struct data_frame {
  uint32_t size;
  uint8_t frame[FRAME_SIZE];
} data_frame;

#endif
