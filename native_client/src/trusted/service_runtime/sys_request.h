/*
 * Copyright 2016 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_NACL_SYS_REQUEST_H_
#define NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_NACL_SYS_REQUEST_H_ 1

#include <stdbool.h>

#include "native_client/src/include/nacl_base.h"
#include "native_client/src/include/portability.h"
#include "native_client/src/trusted/service_runtime/include/machine/_types.h"

EXTERN_C_BEGIN

struct NaClAppThread;
int32_t NaClSysWaitForRequest(struct NaClAppThread *natp,
                              bool                 output_app_label);

EXTERN_C_END
#endif

