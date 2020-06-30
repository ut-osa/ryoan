/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SERVICE_RUNTIME_NACL_SECURITY_H_
#define NATIVE_CLIENT_SERVICE_RUNTIME_NACL_SECURITY_H_ 1

#include <stdbool.h>

#include "native_client/src/include/nacl_base.h"
#include "native_client/src/include/portability.h"
#include "native_client/src/trusted/service_runtime/include/machine/_types.h"

EXTERN_C_BEGIN

#define USER_LABEL_SIZE 64

#define LABEL_USER  0xdeadbeefdeadbeef
#define LABEL_OTHER 0xdeadc0addeadc0ad
#define PUBLIC_KEY  0xcacacacacacacaca

struct NaClLabel {
  // The first 8 bytes of label is type (LABEL) 
  // The second 8 bytes of label is size
  // The rest of label is the actual label
  uint64_t type;
  uint64_t size;
  uint8_t  label[];
};

struct NaClPubKey {
  // The first 8 bytes of key is type () 
  // The second 8 bytes of label is size
  // The rest of label is the actual label
  uint64_t type;
  uint64_t size;
  uint8_t key[];
};

struct NaClApp;

bool NaClAppIsConfined(void);

struct NaClLabel* NaClGetUserLabel(void);

inline uint64_t NaClLabelGetType(struct NaClLabel *label) {
  return label->type;
}

inline uint64_t NaClLabelGetSize(struct NaClLabel *label) {
  return label->size;
}

inline uint8_t* NaClLabelGetLabel(struct NaClLabel *label) {
  return (label->label + sizeof(uint64_t) * 2);
}

inline uint64_t NaClLabelGetTotalSize(struct NaClLabel *label) {
  return sizeof(struct NaClLabel) + NaClLabelGetSize(label) + 2 * sizeof(uint64_t);
}

struct NaClLabel* NaClLabelMake(uint64_t label_size,
                                uint64_t type);

// 0: success; else: failure
int32_t NaClAddLabel(struct NaClLabel *label);

bool NaClHasLabel(struct NaClLabel *label);

void NaClSetUserLabel(void);

// 0: equal; else: different
int32_t NaClCmpLabel(struct NaClLabel *label1, struct NaClLabel *label2);

struct NaClLabel** NaClGetLabel(uint32_t* num_keys, uint32_t **key_sizes);

void NaClLabelFinalize(void);

void NaClSetUserPubKey(struct NaClPubKey *key);

struct NaClPubKey* NaClGetUserPubKey(void);

struct NaClPubKey* NaClPubKeyMake(uint64_t key_size);

inline uint64_t NaClPubKeyGetSize(struct NaClPubKey *key) {
  return key->size;
}

inline uint8_t* NaClPubKeyGetKey(struct NaClPubKey *key) {
  return (key->key + sizeof(uint64_t) * 2);
}

void NaClPubKeyFinalize(void);

EXTERN_C_END

#endif
