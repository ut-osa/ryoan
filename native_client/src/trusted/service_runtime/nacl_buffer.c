/*
 * This file contains class code for NaClBuffer
 */

#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/trusted/service_runtime/buffer_common.h"
#include "native_client/src/trusted/service_runtime/nacl_buffer.h"

static struct NaClRefCountVtbl const kNaClBufferVtbl; /* fwd */
extern uint8_t *GetCurrentBuffer(void **base_tbl, uint32_t pos);
extern int32_t Grow(void **base_tbl, uint32_t needed, uint32_t *size);
extern int32_t BufferRead(void **base_tbl, void *buf, uint32_t buf_len, uint32_t *pos);
extern int32_t BufferWrite(void     **base_tbl,
                           void     *buf,
                           uint32_t buf_len,
                           uint32_t *tbl_len,
                           uint32_t *pos);

int NaClBufferCtor(struct NaClBuffer* bp) {
  memset(bp->base_tbl, 0, BUF_CHUNK_GROUP_SIZE * sizeof(void *));
  bp->size = 0;
  bp->len = 0;
  bp->pos = 0;
  bp->num_item_got = 0;
  bp->num_item_needed = 0;
  bp->need_to_get_total_count = true;
  if (!NaClRefCountCtor(&bp->base)) {
    return 0;
  }
  NACL_VTBL(NaClRefCount, bp) = (struct NaClRefCountVtbl *) &kNaClBufferVtbl;
  return 1;
}

static void  NaClBufferDtor(struct NaClRefCount *nrcp) {
  struct NaClBuffer* bp = (struct NaClBuffer *) nrcp;
  NaClLog(6, "Dtor buffer\n");
  BaseTblDtor(bp->base_tbl);
  nrcp->vtbl = &kNaClRefCountVtbl;
  (*nrcp->vtbl->Dtor)(nrcp);
}

struct NaClBuffer *NaClBufferRef(struct NaClBuffer *bp) {
  return (struct NaClBuffer*) NaClRefCountRef(&bp->base);
}

void NaClBufferUnref(struct NaClBuffer* bp) {
  NaClLog(6, "buffer current count is %d\n", (int)bp->base.ref_count);
  NaClRefCountUnref(&bp->base);
}

void NaClBufferSafeUnref(struct NaClBuffer* bp) {
  if (NULL != bp) {
    NaClLog(6, "buffer current count is %d\n", (int)bp->base.ref_count);
    NaClRefCountUnref(&bp->base);
  }
}

static struct NaClRefCountVtbl const kNaClBufferVtbl = {
  NaClBufferDtor,
};

static inline int32_t grow_if_needed(struct NaClBuffer *bp, uint32_t needed) {
  if (needed > BUF_MAX_LEN)
    return -1;
  return Grow(bp->base_tbl, needed, &bp->size);
}

int32_t NaClBufferRead(struct NaClBuffer *bp, void *buf, uint32_t len) {
  if (bp->len - bp->pos < len) {
    len = bp->len - bp->pos;
  }
  if (len == 0)
    return len;
  return BufferRead(bp->base_tbl, buf, len, &bp->pos);
}

int32_t NaClBufferWrite(struct NaClBuffer *bp, void *buf, uint32_t len) {
  if (grow_if_needed(bp, bp->pos + len) != 0) {
    NaClLog(LOG_ERROR, "Fail at grow if needed\n");
    return -1;
  }
  return BufferWrite(bp->base_tbl, buf, len, &bp->len, &bp->pos);
}
