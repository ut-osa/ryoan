#include <stdbool.h>
#include <stddef.h>
#include <errno.h>
#include <string.h>
#include <sgx_runtime.h>
#include <syscall_shim.h>
#include <sys/mman.h>
#include <sgx.h>

#include "enclave_mman.h"

#undef assert
#define assert(exp, msg...) do {\
    if (!(exp)) DIE(msg);\
} while(0)

#define NOT_PAGE_ALIGNED(val) (val & (PAGE_SIZE -1))

typedef struct encl_vma {
    encl_addr_t addr;
    ulong n_pages;
    unsigned int prot;
    bool in_use;
    bool special;
    bool phys_backed;
    struct encl_vma *next;
    struct encl_vma *prev;
} encl_vma;

#define VMA_PAGES 1024
#define MAX_VMAS ((PAGE_SIZE * VMA_PAGES)/sizeof(encl_vma))

#define VMA_SIZE(vma) (vma->n_pages * PAGE_SIZE)

static encl_vma *vma_list_root;
/* Lock for mm ops */
sgx_mutex mm_lock;

static inline
bool vma_contains(encl_vma *vma, encl_addr_t addr)
{
    return (addr >= vma->addr && addr < vma->addr + vma->n_pages * PAGE_SIZE);
}

/* These functions make it easy to adapt if in the future we do something like
   add a secondary tree structure for address lookups */
static inline
encl_vma *vma_addr_next(encl_vma *vma, encl_addr_t addr){ return vma->next; }
static inline
encl_vma *vma_next(encl_vma *vma){ return vma->next; }

static encl_vma *find_unused_vma(ulong size)
{
    encl_vma *vma = vma_list_root;
    ulong n_pages = size >> PAGE_SHIFT;
    if (!n_pages) {
        return NULL;
    }
    while(vma) {
        if (!vma->in_use && vma->n_pages >= n_pages)
            break;
        vma = vma_next(vma);
    }
    return vma;
}

/*
 * Find the VMA addr lives in
 * if addr does not fall within a vma it is not an address that we manage
 * @param addr - address in question
 *
 * @return - address containing vma or NULL if no such vma exists
 */
static encl_vma *find_vma(encl_addr_t addr)
{
    encl_vma *vma = NULL;
    if (NOT_PAGE_ALIGNED(addr)) goto done;
    vma = vma_list_root;
    while (vma && !vma_contains(vma, addr)) {
        vma = vma_addr_next(vma, addr);
    };
done:
    return vma;
}

static
long accept_page(unsigned long vaddr, unsigned long flags, PageType pt)
{

    long ret = 0;
    SecInfo align64 secinfo = {0};
    SecInfo *secinfop = &secinfo;

    secinfo.flags = flags | (pt << SEC_PT_OFFSET);

    if (in_sgx) {
        asm volatile (
            "movq %1, %%rbx\n\t"
            "movq %2, %%rcx\n\t"
            "movq %3, %%rax\n\t"
            "enclu\n\t"
            "movq %%rax, %0\n\t" : "=m"(ret)
                                 : "m"(secinfop), "m"(vaddr), "i"(EACCEPT)
                                 : "memory", "cc", "rax", "rbx", "rcx");
    }

    return ret;
}

bool scan_vmas(void)
{
    encl_vma *vma = vma_list_root;
    ulong totalsize = 0;
    ulong size;
    while (vma) {
        size = VMA_SIZE(vma);
        totalsize += size;
        if (vma->next)
            assert(vma->next->addr == vma->addr + size);
        vma = vma->next;
    }
    assert(totalsize == enclave_params.size - enclave_params.mmap_line);
    return true;
}

static
long accept_region(unsigned long addr, unsigned long size, unsigned long prot,
        PageType pt)
{
    long ret = 0;
    addr &= ~0xFFFUL;
    size = ROUND_UP(size);
    while (size > 0) {
        ret = accept_page(addr, prot, pt);
        if (ret) {
            break;
        }
        addr += PG_SIZE;
        size -= PG_SIZE;
    }
    return ret;
}

static
ulong do_munmap(ulong addr, ulong size)
{
    long ret;
    if (in_sgx) {
        ret = PASS_THROUGH_SYSCALL(enclave_release, 2, addr, size);
        if (ret) {
            DIE("enclave_release failed");
        }

        /*
         * XXX The emulator wont let us call accept_region unless the page is
         * accessable, probably something that can (should) be fixed, but this
         * will have to do for now
         */
        if (PASS_THROUGH_SYSCALL(mprotect, 3, addr, size, PROT_READ|PROT_WRITE)) {
            DIE("mprotect failed!");
        }
        ret = PASS_THROUGH_SYSCALL(munmap, 2, addr, size);
    } else {
        if (accept_region(addr, size, SGX_MODIFIED, PT_TRIM)) {
            DIE("accept_region failed");
        }
        if (PASS_THROUGH_SYSCALL(munmap, 2, addr, size)) {
            DIE("mprotect failed!");
        }
        ret = 0;
    }
    return ret;
}

static inline
void insert(encl_vma *vma, encl_vma *new_vma)
{
    assert(vma->addr + VMA_SIZE(vma) == new_vma->addr,
            "insert inconsistant vma");
    new_vma->next = vma->next;
    new_vma->prev = vma;
    if (new_vma->next)
        new_vma->next->prev = new_vma;
    vma->next = new_vma;
}

static encl_vma *freelist[MAX_VMAS];
static encl_vma **freelist_head;

static inline
encl_vma **populate_freelist(void)
{
    static encl_vma vma_storage[MAX_VMAS];
    int i;
    for (i = 0; i < MAX_VMAS; ++i)
    {
        freelist[i] = vma_storage + i;
    }
    return freelist + i - 1;
}

static
encl_vma *alloc_vma(void)
{
    if (!freelist_head)
        freelist_head = populate_freelist();
    assert(freelist_head >= freelist, "vma freelist exausted");
    return *(freelist_head--);
}

static
void free_vma(encl_vma *vma)
{
    assert(freelist_head, "free vma called before alloc");
    assert(freelist_head < freelist + MAX_VMAS - 1);
    *(++freelist_head) = vma;
}

static
ulong back_vma(encl_vma *vma)
{
    ulong ret = 0;
    ulong addr = vma->addr;
    ulong size = VMA_SIZE(vma);
    if (in_sgx) {
        ret = PASS_THROUGH_SYSCALL(enclave_aug, 5, addr, size,
                                   PROT_READ|PROT_WRITE, -1, 0);
    } else {
        ret = PASS_THROUGH_SYSCALL(mmap, 6, addr, size, PROT_READ|PROT_WRITE,
                MAP_ANONYMOUS|MAP_PRIVATE|MAP_FIXED, -1, 0);
    }

    if (ret != addr) {
        DIE("enclave aug failed\n");
        goto done;
    }

    ret = accept_region(addr, size, SGX_READ|SGX_WRITE|SGX_PENDING, PT_REG);
    if (ret) {
        DIE("accept_region failed\n");
        goto done;
    }

    /* TODO we have to fix sgx's notion of permissions as well */

    vma->phys_backed = true;
    vma->prot = PROT_READ|PROT_WRITE;
done:
    return ret;
}

static inline
ulong vma_setprot(encl_vma *vma, unsigned prot)
{
    ulong ret = 0;
    if (prot == vma->prot) {
        return 0;
    }

    if (!vma->phys_backed) {
        assert(prot == PROT_NONE);
    } else {
        /* TODO update sgx's notion of prot */
        /* XXX EMODPE One for each page */
        ret = PASS_THROUGH_SYSCALL(mprotect, 3, vma->addr, VMA_SIZE(vma), prot);
    }

    if (!ret) vma->prot = prot;
    return ret;
}

static
void disconnect(encl_vma *vma)
{
    vma->prev->next = vma->next;
    vma->next->prev = vma->prev;
}

static
void merge(encl_vma *low_vma, encl_vma *high_vma)
{
    assert(VMA_SIZE(low_vma) + low_vma->addr == high_vma->addr,
            "merge inconsistent VMA");
    low_vma->n_pages += high_vma->n_pages;
    disconnect(high_vma);
    free_vma(high_vma);
}

static inline
void try_merge(encl_vma *low_vma, encl_vma *high_vma)
{
    if (!low_vma || !high_vma)
        return;
    assert(low_vma->next == high_vma, "weird merge");

    if (low_vma->in_use !=  high_vma->in_use)
        return;

    if (low_vma->phys_backed !=  high_vma->phys_backed)
        return;

    if (low_vma->special !=  high_vma->special)
        return;

    /* if both unused we don't care about prot */
    if (low_vma->in_use && low_vma->prot != high_vma->prot)
        return;


    /* ok, let's merge (high_vma is freed) */
    merge(low_vma, high_vma);
}


/*
 * Set the protection bits on a vma, do allocation if needed for the new
 * permissions (or free if not needed)
 * @param vma - vma to adjust
 * @prot - desired protection bits
 *
 * @return 0 on success else -ERRNO
 */
static
int claim_vma(encl_vma *vma, unsigned prot)
{
    /* in some places we use this to update the vma protection
       so if a vma is already claimed we don't care */
    int ret = 0;
    if (prot == PROT_NONE) {
        /* free phys backing if we need to */
        /*
        if (vma->phys_backed) {
            ret = do_munmap(vma->addr, VMA_SIZE(vma));
            if (ret) DIE("do_unmmap failed");
            vma->phys_backed = false;
        }
        */
    } else {
        if (!vma->phys_backed) {
            ret = back_vma(vma);
            if (ret) DIE("back_vma failed");
        }
    }
    if (!ret) {
        ret = vma_setprot(vma, prot);
        vma->in_use = true;
    }
    try_merge(vma, vma->next);
    try_merge(vma->prev, vma);
    return 0;
}

static inline
encl_vma *split_vma(encl_vma *vma, encl_addr_t addr)
{
    encl_vma *new_vma;
    if (vma->addr != addr || !vma_contains(vma, addr)) {
        new_vma = alloc_vma();
        memcpy(new_vma, vma, sizeof(encl_vma));
        new_vma->addr = addr;
        new_vma->n_pages = (vma->addr + VMA_SIZE(vma) - addr) >> PAGE_SHIFT;

        vma->n_pages = ((addr - vma->addr) >> PAGE_SHIFT);

        insert(vma, new_vma);
    } else {
        new_vma = vma;
    }
    return new_vma;
}

/*
 * Returns a pointer to a vma of size at most size, which starts at addr
 *
 * vma is broken apart if necessary to accommodate
 */
static encl_vma *dissect_vma(encl_vma *vma, encl_addr_t addr, ulong size)
{
    vma = split_vma(vma, addr);
    split_vma(vma, addr + size);
    return vma;
}

/*
 * allocate size bytes
 * @param size - number of bytes requested
 * @param prot - standard page protections
 * @param specail - area is for something lower level than the application which
 *                  we don't want relcaimed or merged (like a  TCS)
 *
 * @return address on success else -ERRNO
 */
encl_addr_t allocate_region(ulong size, unsigned prot, bool special)
{
    encl_addr_t ret = -EINVAL;
    encl_addr_t addr;
    encl_vma *vma;

    if (NOT_PAGE_ALIGNED(size))
        goto done;
    ret = -ENOMEM;
    vma = find_unused_vma(size);
    if (!vma)
        goto done;

    addr = vma->addr;
    assert(!vma->in_use);
    assert(!vma->phys_backed);

    /* dissect creates new vma if more than size is available */
    vma = dissect_vma(vma, addr, size);
    vma->special = special;
    ret = claim_vma(vma, prot);
    if (ret != 0)
        goto done;

    ret = addr;
done:
    return ret;
}

/*
 * allocate size bytes at some addr, willingly displace whatever lives there
 * @param addr - requested address
 * @param size - number of bytes requested
 * @param prot - standard page protections
 *
 * @return address on success else -ERRNO
 */
encl_addr_t allocate_region_at(encl_addr_t addr, ulong size, unsigned prot)
{
    encl_addr_t _addr, ret = -EINVAL;
    encl_vma *vma;

    if (NOT_PAGE_ALIGNED(size))
        goto done;

    ret = free_region(addr, size);
    if (ret)
        goto done;
    _addr = addr;

    vma = find_vma(_addr);
    while (vma && size) {
        vma = dissect_vma(vma, _addr, size);
        assert(!vma->in_use);
        assert(!vma->phys_backed);
        if (claim_vma(vma, prot)) {
            DIE("claim_vma failed");
        }
        _addr += VMA_SIZE(vma);
        size -= VMA_SIZE(vma);
        vma = find_vma(addr);
    }

    if (size) {
        DIE ("Ran out of mem");
        goto done;
    }

    ret = addr;

done:
    return ret;
}


/*
 * Update the protection bits on a region
 * @param addr - first addr
 * @param size - length of region (should be a multiple of PAGE_SIZE)
 * @param prot - desired protection bits
 *
 * @return 0 on success else -ERRNO
 */
int update_prot(encl_addr_t addr, ulong size, unsigned prot)
{
    int ret = -EINVAL;
    encl_vma *vma;

    if (NOT_PAGE_ALIGNED(size)) goto done;
    vma = find_vma(addr);
    while (vma && size) {
        vma = dissect_vma(vma, addr, size);
        if (claim_vma(vma, prot))
            DIE("claim_vma failed");
        addr += VMA_SIZE(vma);
        size = (size > VMA_SIZE(vma) ? size - VMA_SIZE(vma) : 0);
        vma = find_vma(addr);
    }
    ret = 0;
done:
    return ret;
}

static inline
void mark_unused(encl_vma *vma)
{
    vma->in_use = false;
}

static inline
ulong unmap_phys(encl_vma *vma)
{
    ulong ret = 0;
    if (!vma->phys_backed) {
        return 0;
    }
    ret = do_munmap(vma->addr, VMA_SIZE(vma));
    vma->phys_backed = false;
    return ret;
}

static
void discard_vma(encl_vma *vma)
{
    ulong ret;
    ret  = unmap_phys(vma);
    if (ret) DIE("Unmap failed\n");

    mark_unused(vma);
    try_merge(vma, vma->next);
    try_merge(vma->prev, vma);
}

static
void unclaim_vma(encl_vma *vma)
{
    vma->in_use = false;
    discard_vma(vma);
}

/*
 * split vma if necessary return at most size to the pool
 * @param vma - encl_vma which contains the address we are freeing
 * @param addr - address to begin freeing at
 * @param size - amount of memory to free (may free less if we hit upon the vma
 *               boarder
 *
 * @return amount freed - useful, especially if less than size
 */
static
ulong relinquish_vma(encl_vma *vma, encl_addr_t addr, ulong size)
{
    ulong freed = 0;
    assert(vma->in_use, "Tried to free unused VMA");
    if (!vma_contains(vma, addr))
        goto done; /* that was easy! */

    vma = dissect_vma(vma, addr, size);
    freed = VMA_SIZE(vma);
    unclaim_vma(vma);
done:
    return freed;
}

long commit_tcs(SGX_tcs *tcs)
{
    long ret = PASS_THROUGH_SYSCALL(enclave_make_tcs, 1, tcs);
    if (ret)
        goto done;
    ret = accept_page((ulong)tcs, SGX_MODIFIED, PT_TCS);
done:
    return ret;
}


/*
 * Free size bytes at address
 * @param size - number of bytes to free (Should be a multiple of PAGE_SIZE)
 * @param prot - standard page protections
 *
 * @return 0 on success else -ERRNO
 */
int free_region(encl_addr_t addr, ulong size)
{
    int ret = -EINVAL;
    ulong freed;
    encl_vma *vma = find_vma(addr);
    if (!vma || NOT_PAGE_ALIGNED(size)) goto done;

    do {
        if (vma->in_use) {
            freed = relinquish_vma(vma, addr, size);
        } else {
            freed = VMA_SIZE(vma) - (addr - vma->addr);
        }
        addr += freed;
        size -= freed;
    } while (size && (vma = find_vma(addr)));

    ret = 0;
done:
    return ret;
}

ulong init_encl_mman(encl_addr_t addr, ulong size)
{
    ulong ret = 0;
    vma_list_root = alloc_vma();
    vma_list_root->addr = addr;
    vma_list_root->n_pages = size >> PAGE_SHIFT;
    vma_list_root->prot = PROT_NONE;
    vma_list_root->in_use = false;
    vma_list_root->special = true;
    vma_list_root->phys_backed = false;
    vma_list_root->next = NULL;
    vma_list_root->prev = NULL;
    return ret;
}
