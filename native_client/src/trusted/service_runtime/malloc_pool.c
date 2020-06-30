#include "native_client/src/trusted/service_runtime/malloc_pool.h"
#include "native_client/src/trusted/service_runtime/include/sys/mman.h"
#include "native_client/src/trusted/service_runtime/nacl_config.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"
#include "native_client/src/trusted/service_runtime/sys_memory.h"
#include "native_client/src/include/nacl_assert.h"

#include <string.h>

#define POOL_PROT  (NACL_ABI_PROT_READ | NACL_ABI_PROT_WRITE)
#define POOL_FLAGS (NACL_ABI_MAP_ANONYMOUS | NACL_ABI_MAP_PRIVATE)

static inline
uintptr_t PoolPageToAddr(struct SgxMallocPool *pool, uintptr_t usrpage)
{
    return ((usrpage - 1) << NACL_PAGESHIFT) + pool->start;
}

static inline
uintptr_t AddrToPoolPage(struct SgxMallocPool *pool, uintptr_t usraddr)
{
    return ((usraddr - pool->start) >> NACL_PAGESHIFT) + 1;
}

int32_t NaClInitMallocPool(struct NaClApp *nap)
{
    struct NaClVmmap *map;
    size_t pool_pages;
    uintptr_t usraddr;

    /* Sanity, don't do this twice */
    ASSERT(nap->malloc_pool.start == 0);
    ASSERT(nap->malloc_pool.size % NACL_PAGESIZE == 0);

    map = &nap->malloc_pool.map;
    pool_pages = nap->malloc_pool.size >> NACL_PAGESHIFT;

    /*
     * SGX malloc pool for confined applications
     * We carve our a section of the address space and use this mini
     * address space to manage it
     */
    if (!NaClVmmapCtor(map)) {
        NaClLog(LOG_ERROR, "ERROR NaClVmmapCtor for sgx_malloc_pool\n");
        return -1;
    }

    /* The find space function uses 0 to report an error so we'll allocate a
       page there */
    NaClVmmapAdd(map, 0, 1, NACL_ABI_PROT_NONE, 0, NULL, 0, 0);

    /*
     * When looking for holes, starts at highest address and works its way down,
     * so we allocate a dummy page to establish size
     * the space between the dummy pages should be equal to the requested size
     */
    NaClVmmapAdd(map, pool_pages + 1, 1, NACL_ABI_PROT_NONE, 0, NULL, 0, 0);

    /* Now we just need a real chunk of pages to manage */
    usraddr = NaClSysMmapIntern(nap, NULL, pool_pages << NACL_PAGESHIFT,
                                           POOL_PROT, POOL_FLAGS, -1, 0);

    if (NaClPtrIsNegErrno(&usraddr)) {
        NaClLog(LOG_ERROR, "ERROR  Alloc sgx_malloc_pool failed\n");
        return -1;
    }
    NaClLog(LOG_INFO, "malloc pool starts at %lx goes to %lx\n", usraddr,
            usraddr + (pool_pages << NACL_PAGESHIFT));

    nap->malloc_pool.start = usraddr;

    return 0;
}

void ClearMallocPoolVisitor(void *state, struct NaClVmmapEntry *entry)
{
    struct NaClApp *nap = (struct NaClApp *)state;
    struct SgxMallocPool *pool = &nap->malloc_pool;
    uintptr_t _addr, npages = pool->size >> NACL_PAGESHIFT;
    void *addr;

    /* our guard pages */
    if (entry->page_num >= npages || !entry->page_num)
        return;
    _addr = PoolPageToAddr(pool, entry->page_num);
    addr = (void *)NaClUserToSysAddrNullOkay(nap, _addr);
    memset(addr, 0, entry->npages << NACL_PAGESHIFT);
}

uint32_t NaClFreeMallocPoolChunk(struct SgxMallocPool *pool, uintptr_t uaddr,
                                 size_t len) {
    uintptr_t page = AddrToPoolPage(pool, uaddr);
    NaClVmmapRemove(&pool->map, page, len >> NACL_PAGESHIFT);
    return 0;
}

void NaClClearMallocPool(struct NaClApp *nap) {
   /*
    NaClVmmapVisit(&nap->malloc_pool.map, ClearMallocPoolVisitor, nap);
    */
    NaClVmmapRemove(&nap->malloc_pool.map, 1,
                    nap->malloc_pool.size >> NACL_PAGESHIFT);
}


/*
 * Service mmap requests of confined apps
 */
uintptr_t NaClGetMallocPoolChunk(struct SgxMallocPool *pool, size_t len)
{
    uintptr_t usrpage;
    size_t npages = len >> NACL_PAGESHIFT;
    uintptr_t usraddr = 0;
    int prot = NACL_ABI_PROT_READ | NACL_ABI_PROT_WRITE;
    struct NaClVmmap *map = &pool->map;

    usrpage = NaClVmmapFindMapSpace(map, npages);
    if (usrpage) {
        NaClVmmapAddWithOverwrite(map, usrpage, npages, prot, 0, NULL, 0, 0);
        usraddr = PoolPageToAddr(pool, usrpage);
    }
    if (!usraddr) {
        NaClLog(LOG_FATAL, "Ran out of malloc pool\n");
    }
    return usraddr;
}
