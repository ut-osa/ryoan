/*
 * Copyright (c) 2016 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_FAKE_SELF_LABEL_H_
#define NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_FAKE_SELF_LABEL_H_ 1

#include <stdint.h>
#include "native_client/src/include/build_config.h"
#include "native_client/src/include/nacl_base.h"
#include "native_client/src/include/portability.h"

EXTERN_C_BEGIN

struct NaClLabel;
struct NaClApp;

struct NaClLabel* GetFakeAppLabel(struct NaClApp *nap);
void FakeAppLabelFinalize(void);

EXTERN_C_END

#endif
