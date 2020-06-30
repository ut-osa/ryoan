/*
 * Copyright (c) 2016 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_NACL_READBUFFER_OP_H_
#define NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_NACL_READBUFFER_OP_H_

#include "native_client/src/include/nacl_base.h"

EXTERN_C_BEGIN

struct NaClApp;
struct redir;

int32_t GetAllInputFromPipe(struct NaClApp *nap,
                            struct redir   *redir_queue,
                            int32_t        num_wait_for);

EXTERN_C_END

#endif
