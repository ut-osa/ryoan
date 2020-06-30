/*
 * Copyright (c) 2016 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_FAKE_KEY_H_
#define NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_FAKE_KEY_H_ 1

#include <stdint.h>
#include "native_client/src/include/build_config.h"
#include "native_client/src/include/nacl_base.h"
#include "native_client/src/include/portability.h"

EXTERN_C_BEGIN

#define FAKE_KEY_HEX "b52c505a37d78eda5dd34f20c22540ea1b58963cf8e5bf8ffa85f9f2492505b4"

uint8_t* GetFakeKey(void);
void FakeKeyFinalize(void);

EXTERN_C_END

#endif
