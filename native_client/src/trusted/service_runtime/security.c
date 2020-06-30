#include "native_client/src/trusted/service_runtime/security.h"
#include "native_client/src/trusted/service_runtime/hashtable.h"
#include "native_client/src/trusted/service_runtime/libsodium/src/libsodium/include/sodium.h"
#include "native_client/src/trusted/service_runtime/security.h"
#include "native_client/src/shared/platform/nacl_log.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

// TODO only 1 thread?

#define USER_LABEL_SIZE 64

static struct cfuhash_table *hash = NULL;

static struct NaClLabel *user_label = NULL;
static struct NaClPubKey *user_public_key = NULL;

extern uint64_t NaClLabelGetType(struct NaClLabel *label);
extern uint64_t NaClLabelGetSize(struct NaClLabel *label);
extern uint8_t* NaClLabelGetLabel(struct NaClLabel *label);
extern uint64_t NaClLabelGetTotalSize(struct NaClLabel *label);
extern uint64_t NaClPubKeyGetSize(struct NaClPubKey *key);
extern uint8_t* NaClPubKeyGetKey(struct NaClPubKey *key);

bool NaClAppIsConfined(void) {
  return hash != NULL || user_label != NULL;
}

struct NaClLabel* NaClGetUserLabel(void) {
  return user_label;
}

struct NaClLabel* NaClLabelMake(uint64_t label_size,
                                uint64_t type) {
  struct NaClLabel* label = (struct NaClLabel *) malloc(sizeof(struct NaClLabel) +
                                                        label_size +
                                                        sizeof(uint64_t) * 2);
  if (NULL == label) {
    NaClLog(LOG_FATAL, "Fail to allocate memory for label\n");
  }
  label->size = label_size;
  label->type = type;
  memcpy(label->label, &type, sizeof(uint64_t));
  memcpy(label->label + sizeof(uint64_t), &label_size, sizeof(uint64_t));
  return label;
}

// 0: equal; else: different
int32_t NaClCmpLabel(struct NaClLabel *label1, struct NaClLabel *label2) {
  uint64_t label1_size;
  uint64_t label2_size;
  if (label1 == NULL) return -1;
  if (label2 == NULL) return 1;
  label1_size = NaClLabelGetSize(label1);
  label2_size = NaClLabelGetSize(label2);
  if (label1_size != label2_size) {
    return label1_size - label2_size;
  }
  return memcmp(NaClLabelGetLabel(label1),
                NaClLabelGetLabel(label2),
                label1_size);
}

// 0: success; else: failure
int32_t NaClAddLabel(struct NaClLabel *label) {
  uint32_t label_size = 0;
  if (NaClLabelGetType(label) == LABEL_USER) {
    if (user_label == NULL) {
      user_label = label;
    } else {
      // sanity check
      if (NaClCmpLabel(user_label, label) != 0) {
        NaClLog(LOG_FATAL, "User only has one label\n");
      }
    }
  } else {
    if (hash == NULL) {
      hash = cfuhash_new_with_initial_size(8); 
    }
    label_size = NaClLabelGetTotalSize(label);
    if (!cfuhash_exists_data(hash, label, label_size)) {
      cfuhash_put_data(hash, label, label_size, NULL, 0, NULL);
    }
  }
  return 0;
}

bool NaClHasLabel(struct NaClLabel *label) {
  uint32_t label_size;
  bool has_label;
  label_size = NaClLabelGetTotalSize(label);
  has_label = (NaClCmpLabel(user_label, label) == 0);
  if (hash != NULL) {
    has_label = has_label || (cfuhash_exists_data(hash, label, label_size) == 1);
  }
  return has_label;
}

void NaClSetUserLabel(void) {
  user_label = NaClLabelMake(USER_LABEL_SIZE, LABEL_USER);
  randombytes_buf(NaClLabelGetLabel(user_label), USER_LABEL_SIZE);
}

// Clear all labels
void NaClLabelClear(void) {
  free(user_label);
  user_label = NULL;
  cfuhash_clear(hash);
}

struct NaClLabel** NaClGetLabel(uint32_t* num_keys, uint32_t **key_sizes) {
  return (struct NaClLabel **) cfuhash_keys_data(hash, num_keys, key_sizes, 1);
}

void NaClLabelFinalize(void) {
  cfuhash_destroy_with_free_fn(hash, free);
  hash = NULL;
  free(user_label);
}

void NaClSetUserPubKey(struct NaClPubKey *key) {
  if (user_public_key == NULL) {
    user_public_key = key;
  } else {
    NaClLog(LOG_FATAL, "User can only propagate one public key\n");
  }
}

struct NaClPubKey* NaClGetUserPubKey(void) {
  return user_public_key;
}

struct NaClPubKey* NaClPubKeyMake(uint64_t key_size) {
  struct NaClPubKey* key = (struct NaClPubKey *) malloc(sizeof(struct NaClPubKey) +
                                                       key_size +
                                                       sizeof(uint64_t) * 2);
  key->type = PUBLIC_KEY;
  key->size = key_size;
  memcpy(key->key, &key->type, sizeof(uint64_t));
  memcpy(key->key + sizeof(uint64_t), &key_size, sizeof(uint64_t));
  return key;
}

void NaClPubKeyFinalize(void) {
  if (!user_public_key)
    free(user_public_key);
}
