#ifndef NATIVE_CLIENT_SERVICE_RUNTIME_NACL_MALLOC_POOL_H_
#define NATIVE_CLIENT_SERVICE_RUNTIME_NACL_MALLOC_POOL_H_ 1
#include "native_client/src/trusted/service_runtime/sel_mem.h"

#include <stdint.h>
#include <stddef.h>

struct NaClApp;
struct SgxMallocPool {
  struct NaClVmmap map;
  uint32_t start;
  size_t size;
};


/* Usr address on success; 0 on failure */
uintptr_t NaClGetMallocPoolChunk(struct SgxMallocPool *pool, size_t len);

/* Usr address on success; 0 on failure */
uint32_t NaClFreeMallocPoolChunk(struct SgxMallocPool *pool, uintptr_t uaddr,
                                 size_t len);

void NaClClearMallocPool(struct NaClApp *nap);

/* 0: Success; else failure */
int32_t NaClInitMallocPool(struct NaClApp *nap);


#endif
