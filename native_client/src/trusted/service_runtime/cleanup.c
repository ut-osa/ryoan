/*
 * Copyright (c) 2016 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>

#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/shared/platform/nacl_sync_checked.h"

#include "native_client/src/trusted/desc/nacl_desc_base.h"
#include "native_client/src/trusted/desc/nacl_desc_io.h"
#include "native_client/src/trusted/service_runtime/file_name.h"
#include "native_client/src/trusted/service_runtime/hashtable.h"
#include "native_client/src/trusted/service_runtime/nacl_buffer.h"
#include "native_client/src/trusted/service_runtime/nacl_file_buffer.h"
#include "native_client/src/trusted/service_runtime/redir.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"

void CleanupNaClBuffer(struct NaClApp    *nap,
                       struct redir      *redir_read_queue,
                       struct redir      *redir_write_queue) {
  struct NaClBuffer* rbp;
  struct NaClBuffer* wbp;
  struct redir *entry;
  int read_buffer_count = 0;
  int write_buffer_count = 0;
  NaClLog(4, "Entering CleanupNaClBuffer\n");
  for (entry = redir_read_queue; NULL != entry; entry = entry->next) {
    read_buffer_count++;
    rbp = NaClAppGetReadBufferByFdSem(nap, entry->u.host.d);
    if (NULL != rbp) {
      NaClAppSetReadBufferSem(nap, entry->u.host.d, NULL);
    }
    NaClBufferSafeUnref(rbp);
  }
  for (entry = redir_write_queue; NULL != entry; entry = entry->next) {
    write_buffer_count++;
    wbp = NaClAppGetWriteBufferByFdSem(nap, entry->u.host.d);
    if (NULL != wbp) {
      NaClAppSetWriteBufferSem(nap, entry->u.host.d, NULL);
    }
    NaClBufferSafeUnref(wbp);
  }
  NaClLog(6, "Should dtor %d read buffer and %d write buffer\n",
          read_buffer_count, write_buffer_count);
}

void CleanupNaClFileBuffer(struct NaClApp   *nap,
                           struct file_name *file_name_queue) {
  struct NaClFileBuffer *fbp;
  struct file_name *entry;
  struct NaClDesc *desc;
  NaClLog(4, "Entering CleanupNaClFileBuffer\n");
  for (entry = file_name_queue; NULL != entry; entry = entry->next) {
    desc = cfuhash_get(nap->file_name_tbl, entry->name);
    if (NULL != desc) {
      cfuhash_put(nap->file_name_tbl, entry->name, NULL);
      fbp = NaClAppGetFileBufferSem(nap, desc);
      if (NULL != fbp) {
        NaClAppSetFileBufferSem(nap, ((struct NaClDescIoDesc *) desc)->hd->d, NULL);
      }
      NaClFileBufferSafeUnref(fbp);
    }
    NaClDescSafeUnref(desc);
  }
  cfuhash_destroy(nap->file_name_tbl);
}

void CleanupNaClDeferDesc(struct NaClApp *nap) {
  int i;
  struct NaClDesc *ndp;
  NaClLog(4, "Entering CleanupNaClDeferDesc\n");
  NaClXMutexLock(&nap->defer_desc_mu);
  NaClLog(6, "Defer count: %d\n", nap->defer_count);
  for (i = 0; i < nap->defer_count; i++) {
    ndp = NaClAppGetDescDeferMu(nap, i);
    if (NULL != ndp) {
      NaClAppSetDescDeferMu(nap, i, NULL);
    }
    NaClDescSafeUnref(ndp);
  }
  NaClXMutexUnlock(&nap->defer_desc_mu);
}
