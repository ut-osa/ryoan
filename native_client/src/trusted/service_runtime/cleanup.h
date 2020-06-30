/*
 * Copyright (c) 2016 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_CLEANUP_H_
#define NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_CLEANUP_H_

struct file_name;
struct redir;
struct NaClApp;

#include "native_client/src/include/nacl_base.h"

void CleanupNaClBuffer(struct NaClApp    *nap,
                       struct redir      *redir_read_queue,
                       struct redir      *redir_write_queue); 

void CleanupNaClFileBuffer(struct NaClApp   *nap,
                           struct file_name *file_name_queue); 

void CleanupNaClDeferDesc(struct NaClApp *nap); 

#endif
