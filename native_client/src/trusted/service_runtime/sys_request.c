/*
 * Copyright 2016 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdbool.h>

#include "native_client/src/trusted/service_runtime/sys_request.h"

#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/trusted/service_runtime/nacl_app_thread.h"
#include "native_client/src/trusted/service_runtime/nacl_buffer.h"
#include "native_client/src/trusted/service_runtime/nacl_readbuffer_op.h"
#include "native_client/src/trusted/service_runtime/nacl_writebuffer_op.h"
#include "native_client/src/trusted/service_runtime/redir.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"
#include "native_client/src/trusted/service_runtime/cow.h"

static void WriteBufferToNextStage(struct NaClApp *nap,
                                   struct redir   *redir_queue,
                                   bool           output_app_label) {
  struct redir           *entry;
  struct NaClBuffer *wbp = NULL;
  NaClLog(4, "Writing output to the next stage pipeline\n");
  for (entry = redir_queue; NULL != entry; entry = entry->next) {
    //NaClLog(LOG_INFO, "Handling write fd %d\n", entry->u.host.d);
    wbp = NaClAppGetWriteBufferByFdSem(nap, entry->u.host.d);
    if (NULL == wbp) {
      NaClLog(LOG_FATAL,
              "Could not find write buffer corresponding to %d\n",
              entry->u.host.d);
    }
    if (!NaClWriteBufferToNextStage(nap,
                                    wbp,
                                    entry->u.host.d,
                                    entry->u.host.factor_a,
                                    entry->u.host.factor_b,
                                    output_app_label)) {
      NaClLog(LOG_FATAL,
              "Fail to send result to next stage pipeline\n");
    }
    wbp->pos = 0;
    NaClBufferUnref(wbp);
  }
}

int32_t NaClSysWaitForRequest(struct NaClAppThread *natp,
                              bool                 output_app_label) {
  int32_t retval = 0;
  struct NaClApp *nap = natp->nap;
  start_cow(nap);
  //NaClLog(LOG_INFO, "NaClSysWaitForRequest(0x%08"NACL_PRIxPTR")\n", (uintptr_t) natp);
  // NaClSemWait(&nap->request_sem);
  WriteBufferToNextStage(nap, nap->redir_write_queue, output_app_label);
  retval = GetAllInputFromPipe(nap, nap->redir_read_queue, nap->num_wait_for);
  start_clock();
  // NaClSemPost(&nap->request_sem);
  return retval;
}
