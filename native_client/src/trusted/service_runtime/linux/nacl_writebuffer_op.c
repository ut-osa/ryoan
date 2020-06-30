/*
 * Copyright (c) 2016 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "native_client/src/trusted/service_runtime/data_format.h"
#include "native_client/src/trusted/service_runtime/fake_app_label.h"
#include "native_client/src/trusted/service_runtime/fake_key.h"
#include "native_client/src/trusted/service_runtime/hashtable.h"
#include "native_client/src/trusted/service_runtime/libsodium/src/libsodium/include/sodium.h"
#include "native_client/src/trusted/service_runtime/nacl_buffer.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"
#include "native_client/src/trusted/service_runtime/security.h"
#include "native_client/src/shared/platform/nacl_log.h"

static int write_frame(void* buf, int len, int fd) {
  uint8_t* base = (uint8_t*) buf;
  int ret = 0;
  while (len > 0) {
    NaClLog(4,
            "write(%d, 0x%08"NACL_PRIxPTR", %d)\n",
            fd, (uintptr_t)base, len);
    ret = write(fd, base, len);
    if (ret < 0) {
      if (errno == EINTR || errno == EAGAIN)
        continue;
      perror("write");
      return -1;
    }
    len -= ret;
    base += ret;
  }
  return 0;
}

static inline void UpdateNonce(struct NaClApp *nap) {
  if (nap->nonce == NULL) {
    nap->nonce = (uint8_t *) malloc(crypto_aead_aes256gcm_NPUBBYTES);
    randombytes_buf(nap->nonce, crypto_aead_aes256gcm_NPUBBYTES);
  } else {
    sodium_increment(nap->nonce, crypto_aead_aes256gcm_NPUBBYTES);
  }
}

static int32_t OutputCtextWithSymKey(struct NaClApp *nap,
                                     int            fd,
                                     uint8_t        *message,
                                     uint64_t       message_len) {
  uint8_t* ctext_buf = NULL;
  uint64_t ctext_len = 0, transmit_size = 0;
  UpdateNonce(nap);
  ctext_buf = (uint8_t *) malloc(message_len + crypto_aead_aes256gcm_ABYTES);
  if (ctext_buf == NULL) {
    NaClLog(LOG_FATAL, "Fail to allocate memory for ciphertext\n");
  }
  crypto_aead_aes256gcm_encrypt(ctext_buf, (unsigned long long *)&ctext_len,
                                message, message_len,
                                NULL, 0,
                                NULL, nap->nonce, GetFakeKey());
  transmit_size = ctext_len + crypto_aead_aes256gcm_NPUBBYTES;
  NaClLog(4, "Write out %lu to next stage\n", transmit_size);
  if (write_frame(&transmit_size, sizeof(uint64_t), fd) < 0) {
    free(ctext_buf);
    return 0;
  }
  if (write_frame(nap->nonce, crypto_aead_aes256gcm_NPUBBYTES, fd) < 0) {
    free(ctext_buf);
    return 0;
  }
  if (write_frame(ctext_buf, ctext_len, fd) < 0) {
    free(ctext_buf);
    return 0;
  }
  free(ctext_buf);
  return 1;
}

static int32_t OutputPlainText(int fd,
                               uint8_t *message,
                               uint64_t message_len) {
  if (write_frame(&message_len, sizeof(uint64_t), fd) < 0) {
    return 0;
  }
  if (write_frame(message, message_len, fd) < 0) {
    return 0;
  }
  return 1;
}

static int32_t OutputLabel(struct NaClApp   *nap,
                           int              fd,
                           struct NaClLabel *label) {
  NaClLog(4, "OutputLabel\n");
  if (nap->no_enc) {
    return OutputPlainText(fd,
                           label->label,
                           NaClLabelGetSize(label) + sizeof(uint64_t) * 2);
  } else {
    return OutputCtextWithSymKey(nap,
                                 fd,
                                 label->label,
                                 NaClLabelGetSize(label) + sizeof(uint64_t) * 2);
  }
}

static int32_t OutputPublicKey(int               fd,
                               struct NaClPubKey *key) {
  NaClLog(4, "OutputPublicKey\n");
  return OutputPlainText(fd, key->key, NaClPubKeyGetSize(key) + sizeof(uint64_t) * 2);
}

static int32_t OutputLabels(struct NaClApp   *nap,
                            int              fd,
                            struct NaClLabel **labels,
                            uint32_t         num_keys,
                            uint32_t         *key_sizes) {
  uint32_t i;
  for (i = 0; i < num_keys; i++) {
    if (NaClLabelGetTotalSize(labels[i]) != key_sizes[i]) {
      NaClLog(LOG_FATAL, "Data corrupted in hashtable\n");
    }
    if (!OutputLabel(nap, fd, labels[i]))
      return 0;
  }
  return 1;
}

static uint8_t* PreparePlaintext(struct NaClBuffer *wbp,
                                        uint32_t          budget) {
  uint8_t *buf = NULL;
  uint32_t buf_pos = 0, base_pos = 0;
  uint64_t len = 0, len_wo_size = 0;
  // Prepare message
  len = wbp->pos + sizeof(uint64_t);
  if (budget < len) {
    NaClLog(LOG_WARNING, "Budget is less than the required output size\n");
    len = budget;
  }
  len_wo_size = len - sizeof(uint64_t);
  buf = (uint8_t *) malloc(budget + crypto_aead_aes256gcm_ABYTES);
  if (buf == NULL) {
    NaClLog(LOG_FATAL, "Fail to allocate memory for plaintext\n");
  }
  // serialize the tree back to flat buffer for encryption
  memcpy(buf, &len_wo_size, sizeof(uint64_t));
  buf_pos = sizeof(uint64_t);
  while (base_pos < len_wo_size) {
    uint8_t *chunk = GetCurrentBuffer(wbp->base_tbl, base_pos);
    uint32_t n = BUF_CHUNK_SIZE - BUF_POS_IN_CHUNK(base_pos);
    if (n > len_wo_size - base_pos)
      n = len_wo_size - base_pos;
    memcpy(buf + buf_pos, chunk, n);
    base_pos += n;
    buf_pos += n;
  }
  if (budget > len) {
    randombytes_buf(buf + buf_pos, budget - len);
  }
  return buf;
}

int NaClWriteBufferToNextStage(struct NaClApp    *nap,
                               struct NaClBuffer *wbp,
                               int               fd,
                               float             factor_a,
                               int               factor_b,
                               bool              output_app_label) {
  uint64_t total_item = 1;
  uint32_t budget = 0;
  struct NaClLabel ** labels;
  uint32_t num_keys;
  uint32_t *key_sizes;
  uint8_t  *buf;

  labels = NaClGetLabel(&num_keys, &key_sizes);

  /*
  NaClLog(LOG_INFO, "Write buffered output to next stage\n");
  NaClLog(LOG_INFO,
          "NaClWriteBufferToNextStage(0x%08"NACL_PRIxPTR", %d)\n",
          (uintptr_t)wbp, fd);
          */
  budget = (uint32_t)lroundf(nap->total_bytes_for_unit * factor_a + factor_b);
  if (budget > 0 && nap->total_bytes_for_unit > 0) {
    if (nap->output_to_user) {
      // check the label
      if (NaClGetUserLabel() == NULL) {
        NaClLog(LOG_FATAL, "Error: User taint is not set\n");
      }
      if (NaClHasLabel(GetFakeAppLabel(nap))) {
        // Have other label other than the app's own label
        if (num_keys > 1) {
          NaClLog(LOG_FATAL, "Cannot output: app's data have other labels\n");
        } else {
          // output
          if (write_frame(&total_item, sizeof(uint64_t), fd) < 0)
            return 0;
          buf = PreparePlaintext(wbp, budget);
          if (nap->no_enc) {
            if (!OutputPlainText(fd, buf, budget)) {
              free(buf);
              return 0;
            }
          } else {
            if (!OutputCtextWithSymKey(nap, fd, buf, budget)) {
              free(buf);
              return 0;
            }
          }
          free(buf);
          wbp->pos = 0;
          wbp->len = 0;
        }
      } else {
        // Have other label other than the app's own label
        if (num_keys > 0) {
          NaClLog(LOG_FATAL, "Cannot output: app's data have other labels\n");
        } else {
          // output
          if (write_frame(&total_item, sizeof(uint64_t), fd) < 0)
            return 0;
          buf = PreparePlaintext(wbp, budget);
          if (nap->no_enc) {
            if (!OutputPlainText(fd, buf, budget)) {
              free(buf);
              return 0;
            }
          } else {
            if (!OutputCtextWithSymKey(nap, fd, buf, budget)) {
              free(buf);
              return 0;
            }
          }
          free(buf);
          wbp->pos = 0;
          wbp->len = 0;
        }
      }
    } else {
      struct NaClPubKey* key;
      struct NaClLabel* user_label = NaClGetUserLabel();
      if (num_keys > 0) {
        total_item += num_keys;
      }
      if (NaClGetUserPubKey() != NULL) {
        total_item += 1;
      }
      if (output_app_label && !NaClHasLabel(GetFakeAppLabel(nap))) {
        total_item += 1;
      }
      if (user_label != NULL) {
        total_item += 1;
      }
      if (write_frame(&total_item, sizeof(uint64_t), fd) < 0)
        return 0;
      if (num_keys > 0) {
        NaClLog(4, "Output labels in table to downstream\n");
        if (!OutputLabels(nap, fd, labels, num_keys, key_sizes))
          return 0;
      }
      if ((key = NaClGetUserPubKey()) != NULL) {
        NaClLog(4, "Output public keys\n");
        OutputPublicKey(fd, key);
      }
      if (output_app_label && !NaClHasLabel(GetFakeAppLabel(nap))) {
        NaClLog(4, "Output app label to downstream\n");
        if (!OutputLabel(nap, fd, GetFakeAppLabel(nap))) {
          return 0;
        }
      }
      if (user_label != NULL) {
        NaClLog(4, "Output user label to downstream\n");
        if (!OutputLabel(nap, fd, user_label))
          return 0;
      }
      buf = PreparePlaintext(wbp, budget);
      // encrypt and send
      if (nap->no_enc) {
        if (!OutputPlainText(fd, buf, budget)) {
          free(buf);
          return 0;
        }
      } else {
        if (!OutputCtextWithSymKey(nap, fd, buf, budget)) {
          free(buf);
          return 0;
        }
      }
      free(buf);
      wbp->pos = 0;
      wbp->len = 0;
    }
  }
  return 1;
}
