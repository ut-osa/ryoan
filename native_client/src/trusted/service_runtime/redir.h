/*
 * Copyright (c) 2016 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_REDIR_H_
#define NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_REDIR_H_

#include "native_client/src/public/imc_types.h"
#include "native_client/src/shared/imc/nacl_imc_c.h"


struct NaClSocketAddress;

struct redir {
  struct redir  *next;
  int           nacl_desc;
  enum {
    HOST_DESC,
    IMC_DESC
  }             tag;
  union {
    struct {
      int d;
      int mode;
      float factor_a;
      int factor_b;
    }                         host;
    NaClHandle                handle;
    struct NaClSocketAddress  addr;
  } u;
};

#endif
