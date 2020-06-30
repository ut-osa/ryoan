/*
 * This file contains class code for NaClFileBuffer
 */

#include "native_client/src/trusted/service_runtime/nacl_file_buffer.h"
#include "native_client/src/trusted/service_runtime/buffer_common.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include <string.h>

static struct NaClRefCountVtbl const kNaClFileBufferVtbl; /* fwd */

int NaClFileBufferCtor_fixed_len(struct NaClFileBuffer *self, int fixed_len) {
  self->buffer = malloc(fixed_len);
  if (!self->buffer)
    return 0;
  memset(self->buffer, 0, fixed_len);
  self->size = fixed_len;
  self->len = fixed_len;
  self->pos = 0;
  self->writable = 0;
  if (!NaClRefCountCtor(&self->base)) {
    return 0;
  }
  NACL_VTBL(NaClRefCount, self)  = (struct NaClRefCountVtbl *) &kNaClFileBufferVtbl;
  return 1;
}

int NaClFileBufferCtor(struct NaClFileBuffer *self) {
  self->buffer = NULL;
  memset(self->base_tbl, 0, BUF_CHUNK_GROUP_SIZE * sizeof(void *));
  self->size = 0;
  self->len = 0;
  self->pos = 0;
  self->writable = 0;
  if (!NaClRefCountCtor(&self->base)) {
    return 0;
  }
  NACL_VTBL(NaClRefCount, self)  = (struct NaClRefCountVtbl *) &kNaClFileBufferVtbl;
  return 1;
}

static void NaClFileBufferDtor(struct NaClRefCount *nrcp) {
  struct NaClFileBuffer* fbp = (struct NaClFileBuffer *) nrcp;

  if (fbp->buffer) {
    NaClLog(6, "Dtor preloaded file\n");
    free(fbp->buffer);
  } else {
    NaClLog(6, "Dtor in-mem file\n");
    BaseTblDtor(fbp->base_tbl);
  }

  nrcp->vtbl = &kNaClRefCountVtbl;
  (*nrcp->vtbl->Dtor)(nrcp);
}

struct NaClFileBuffer *NaClFileBufferRef(struct NaClFileBuffer *fbp) {
  return (struct NaClFileBuffer *) NaClRefCountRef(&fbp->base);
}

void NaClFileBufferUnref(struct NaClFileBuffer* fbp) {
  NaClLog(6, "file buffer current count is %d\n", (int)fbp->base.ref_count);
  NaClRefCountUnref(&fbp->base);
}

void NaClFileBufferSafeUnref(struct NaClFileBuffer* fbp) {
  if (NULL != fbp) {
    NaClLog(6, "file buffer current count is %d\n", (int)fbp->base.ref_count);
    NaClRefCountUnref(&fbp->base);
  }
}

static struct NaClRefCountVtbl const kNaClFileBufferVtbl = {
  NaClFileBufferDtor,
};

static inline int32_t grow_if_needed(struct NaClFileBuffer* fbp, uint32_t needed) {
  if (fbp->buffer) {
    return needed > fbp->size ? -1 : 0;
  }
  if (needed > BUF_MAX_LEN)
    return -1;
  return Grow(fbp->base_tbl, needed, &fbp->size);
}

int32_t NaClFileBufferRead(struct NaClFileBuffer* fbp, void *buf, uint32_t len) {
  if (fbp->len - fbp->pos < len)
    len = fbp->len - fbp->pos;
  if (len == 0)
    return len;
  if (fbp->buffer) {
    memcpy(buf, fbp->buffer + fbp->pos, len);
    fbp->pos += len;
    return len;
  }
  return BufferRead(fbp->base_tbl, buf, len, &fbp->pos);
}

int32_t  NaClFileBufferWrite(struct NaClFileBuffer* fbp, void *buf, uint32_t len) {
  if (grow_if_needed(fbp, fbp->pos + len) != 0) {
    return -1;
  }

  if (fbp->buffer) {
    memcpy(fbp->buffer + fbp->pos, buf, len);
    fbp->pos += len;
    return len;
  }
  return BufferWrite(fbp->base_tbl, buf, len, &fbp->len, &fbp->pos);
}
