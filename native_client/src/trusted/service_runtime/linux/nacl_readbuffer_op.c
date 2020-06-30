/*
 * Copyright (c) 2016 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include "native_client/src/trusted/service_runtime/data_format.h"
#include "native_client/src/trusted/service_runtime/fake_key.h"
#include "native_client/src/trusted/service_runtime/fake_app_label.h"
#include "native_client/src/trusted/service_runtime/libsodium/src/libsodium/include/sodium.h"
#include "native_client/src/trusted/service_runtime/linux/inet_sockets.h"
#include "native_client/src/trusted/service_runtime/nacl_buffer.h"
#include "native_client/src/trusted/service_runtime/redir.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"
#include "native_client/src/trusted/service_runtime/security.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/shared/platform/nacl_semaphore.h"

#define MAXEVENTS 64

static inline int32_t ReadNBytes(int fd, uint8_t* buf, uint64_t len) {
  uint64_t bytes = 0;
  ssize_t count = 0;

  while (bytes < len) {
    count = read(fd, buf + bytes, len - bytes);
    if (count > 0) {
      NaClLog(4, "Getting %d bytes\n", (int)count);
      bytes += (uint64_t) count;
    } else {
      if (errno == EINTR || errno == EAGAIN) {
        continue;
      } else {
        return -1;
      }
    }
  }
  return 0;
}

int32_t GetAllInputFromPipe(struct NaClApp *nap,
                            struct redir   *redir_queue,
                            uint32_t        num_wait_for) {
  struct redir          *entry;
  struct NaClBuffer *rbp = NULL;
  int32_t efd, s, n, i;
  uint32_t num_err_fd;
  uint64_t total_item_for_this_unit;
  struct epoll_event event;
  struct epoll_event *events;
  int retval = 0;
  /*
  NaClLog(1, "GetAllInputFromPipe(0x%08"NACL_PRIxPTR", 0x%08"NACL_PRIxPTR")\n",
          (uintptr_t)nap, (uintptr_t)redir_queue);
  NaClLog(1, "Waiting on the pipe to fill read buffer\n");
  */
  efd = epoll_create1(0);
  if (efd == -1) {
    NaClLog(LOG_ERROR, "epoll_create: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }
  for (entry = redir_queue; NULL != entry; entry = entry->next) {
    s = makeFdNonBlocking(entry->u.host.d);
    if (s == -1) {
      NaClLog(LOG_ERROR, "makeFdNonBlocking: %s\n", strerror(errno));
      exit(EXIT_FAILURE);
    }
    event.data.fd = entry->u.host.d;
    event.events = EPOLLIN | EPOLLET;
    s = epoll_ctl(efd, EPOLL_CTL_ADD, entry->u.host.d, &event);
    if (s == -1) {
      NaClLog(LOG_ERROR, "epoll_ctl: %s\n", strerror(errno));
      exit(EXIT_FAILURE);
    }
  }
  events = calloc(MAXEVENTS, sizeof event);
  num_err_fd = 0;
  while (1) {
    //NaClLog(LOG_INFO, "Before epoll wait\n");
    n = epoll_wait(efd, events, MAXEVENTS, -1);
    //NaClLog(LOG_INFO, "Getting %d events\n", n);
    for (i = 0; i < n; ++i) {
      if (((events[i].events & EPOLLERR) ||
           (events[i].events & EPOLLHUP)) &&
          (!(events[i].events & EPOLLIN))) {
        NaClLog(LOG_WARNING, "epoll: %s, 0x%x\n", strerror(errno), events[i].events);
        /* close(events[i].data.fd); */
        num_err_fd ++;
        if (num_err_fd == num_wait_for) {
          retval = -1;
          goto done;
        }
      } else {
        rbp = NaClAppGetReadBufferByFdSem(nap, events[i].data.fd);
        // rbp should not be NULL. Something wrong.
        if (NULL == rbp) {
          NaClLog(LOG_ERROR,
                  "NaClAppGetReadBufferByFdSem(%d) returns NULL\n",
                  events[i].data.fd);
          exit(EXIT_FAILURE);
        }
        // Get the total number of items in this work unit
        if (rbp->need_to_get_total_count) {
          rbp->pos = 0;
          rbp->len = 0;
          retval = ReadNBytes(events[i].data.fd,
                              (uint8_t *)&total_item_for_this_unit,
                              sizeof(uint64_t));
          if (retval == 0) {
            rbp->need_to_get_total_count = false;
            rbp->num_item_needed = total_item_for_this_unit;
            rbp->num_item_got = 0;
            nap->total_bytes_for_unit = 0;
            /*
            NaClLog(LOG_INFO, "Total item: %lu\n", total_item_for_this_unit);
            */
          } else{
            NaClLog(LOG_ERROR, "Fail to read the total size\n");
            break;
          }
        }
        while (1) {
          uint64_t transmission_size = 0, used_size = 0, ctext_len = 0;
          uint8_t *ctext_buf = NULL;
          uint8_t *buf = NULL;
          uint64_t plain_text_size;
          uint8_t nonce[crypto_aead_aes256gcm_NPUBBYTES];
          // Get the size of the item
          retval = ReadNBytes(events[i].data.fd,
                              (uint8_t *)&transmission_size,
                              sizeof(uint64_t));
          if (retval == 0) {
             /*
            NaClLog(-1, "Transmission size is: %lu\n", transmission_size);
            */
            // Get user's public key
            if (transmission_size == PUBLIC_KEY) {
              struct NaClPubKey *pub_key;
              uint64_t public_key_size = 0;
              if (ReadNBytes(events[i].data.fd,
                             (uint8_t *)&public_key_size,
                             sizeof(uint64_t)) < 0) {
                NaClLog(LOG_ERROR, "Fail to read public key size\n");
                retval = -1;
                goto done;
              }
              pub_key = NaClPubKeyMake(public_key_size);
              if (ReadNBytes(events[i].data.fd,
                             NaClPubKeyGetKey(pub_key),
                             public_key_size) < 0) {
                NaClLog(LOG_ERROR, "Fail to read public key size\n");
                retval = -1;
                goto done;
              }
              NaClSetUserPubKey(pub_key);
            } else {
              if (nap->no_enc) {
                buf = (uint8_t *) malloc(transmission_size);
                if (buf == NULL) {
                  NaClLog(LOG_FATAL, "No memory for data\n");
                }
                if (ReadNBytes(events[i].data.fd, buf, transmission_size) < 0) {
                  NaClLog(LOG_ERROR, "Fail to read data\n");
                  free(buf);
                  retval = -1;
                  goto done;
                }
                plain_text_size = transmission_size;
              } else {
                if (transmission_size < crypto_aead_aes256gcm_NPUBBYTES) {
                  NaClLog(LOG_FATAL, "Get wrong transmit size\n");
                }
                // Read in nonce
                if (ReadNBytes(events[i].data.fd,
                               nonce,
                               crypto_aead_aes256gcm_NPUBBYTES) < 0) {
                  NaClLog(LOG_ERROR, "Fail to read nonce\n");
                  retval = -1;
                  goto done;
                }
                // Allocate buffer for plain text and ciphertext
                ctext_len = transmission_size - crypto_aead_aes256gcm_NPUBBYTES;
                if (ctext_len < crypto_aead_aes256gcm_ABYTES) {
                  NaClLog(LOG_FATAL, "Message forged\n");
                }
                ctext_buf = (uint8_t *) malloc(ctext_len);
                if (ctext_buf == NULL) {
                  NaClLog(LOG_FATAL, "No memory to read in this item\n");
                }
                buf = (uint8_t *) malloc(ctext_len);
                if (buf == NULL) {
                  NaClLog(LOG_FATAL, "No memory for plain text\n");
                }
                // Read in ciphertext
                if (ReadNBytes(events[i].data.fd, ctext_buf, ctext_len) < 0) {
                  NaClLog(LOG_ERROR, "Fail to read ciphertext\n");
                  free(buf);
                  free(ctext_buf);
                  retval = -1;
                  goto done;
                }
                // decryption
                if (crypto_aead_aes256gcm_decrypt(buf, (unsigned long long *)&plain_text_size,
                                                  NULL,
                                                  ctext_buf, ctext_len,
                                                  NULL, 0, nonce, GetFakeKey()) < 0) {
                  NaClLog(LOG_FATAL, "Message forged!");
                }
              }
              // Get the actual useful size
              used_size = ((uint64_t *)buf)[0];
              if (used_size == LABEL_USER || used_size == LABEL_OTHER) {
                uint64_t label_size = 0;
                struct NaClLabel *label = NULL;
                NaClLog(4, "Get label from upstream\n");
                label_size = ((uint64_t *)buf)[1];
                if (plain_text_size - 2 * sizeof(uint64_t) != label_size) {
                  NaClLog(LOG_FATAL, "Label is forged");
                }
                label = NaClLabelMake(label_size, used_size);
                memcpy(NaClLabelGetLabel(label), buf + sizeof(uint64_t) * 2, label_size);
                // replace fake app label with the application's own label
                if (NaClAddLabel(label) != 0) {
                  NaClLog(LOG_FATAL, "Fail to add label\n");
                }
              } else {
                if (used_size > plain_text_size) {
                  NaClLog(LOG_FATAL, "Useful size is larger than expected\n");
                }
                nap->total_bytes_for_unit += used_size;
                retval = NaClBufferWrite(rbp, buf + sizeof(uint64_t), used_size);
                if (retval < 0) {
                  NaClLog(LOG_FATAL, "Fail to write into buffer\n");
                }
              }
              free(buf);
              free(ctext_buf);
            }
            rbp->num_item_got += 1;
            if (rbp->num_item_got == rbp->num_item_needed) {
               /*
              NaClLog(LOG_INFO, "Get all items for this work unit\n");
              */
              rbp->need_to_get_total_count = true;
              nap->num_finished++;
              s = epoll_ctl(efd, EPOLL_CTL_DEL, events[i].data.fd, &events[i]);
              if (nap->num_finished == num_wait_for) {
                 /*
                NaClLog(LOG_INFO, "Get all frames from source\n");
                */
                if (nap->get_user_input) {
                  // Need to set label for user
                  /*
                  NaClLog(LOG_INFO, "Set label for user data\n");
                  */
                  if (NaClGetUserLabel() == NULL) {
                    NaClSetUserLabel();
                  }
                }
                nap->num_finished = 0;
                retval = 0;
                goto done;
              }
              break;
            }
          } else {
            break;
          }
        }
        NaClBufferSafeUnref(rbp);
      }
    }
  }
 done:
  // reset pos for read
  for (entry = redir_queue; NULL != entry; entry = entry->next) {
    rbp = NaClAppGetReadBufferByFdSem(nap, entry->u.host.d);
    rbp->pos = 0;
    NaClBufferUnref(rbp);
  }
  free(events);
  close(efd);
  return retval;
}
