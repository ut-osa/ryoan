#ifndef _ENCLAVE_MMAN_H_
#define _ENCLAVE_MMAN_H_

#include <stdint.h>
#include <sgx_runtime.h>

typedef uint64_t encl_addr_t;
typedef uint64_t ulong;

/* lock must be held for any operation */
extern sgx_mutex mm_lock;

ulong init_encl_mman(encl_addr_t addr, ulong size);

/*
 * allocate size bytes
 * @param size - number of bytes requested
 * @param prot - standard page protections
 * @param specail - area is for something lower level than the application which
 *                  we don't want relcaimed or merged (like a  TCS)
 *
 * @return address on success else -ERRNO
 */
encl_addr_t allocate_region(ulong size, unsigned prot, bool special);

/*
 * allocate size bytes at some addr, willingly displace whatever lives there
 * @param addr - requested address
 * @param size - number of bytes requested
 * @param prot - standard page protections
 *
 * @return address on success else -ERRNO
 */
encl_addr_t allocate_region_at(encl_addr_t addr, ulong size, unsigned prot);

/*
 * Free size bytes at address
 * @param size - number of bytes to free (Should be a multiple of PAGE_SIZE)
 * @param prot - standard page protections
 *
 * @return 0 on success else -ERRNO
 */
int free_region(encl_addr_t addr, ulong size);

/*
 * Update the protection bits on a region
 * @param addr - first addr
 * @param size - length of region (should be a multiple of PAGE_SIZE)
 * @param prot - desired protection bits
 *
 * @return 0 on success else -ERRNO
 */
int update_prot(encl_addr_t addr, ulong size, unsigned prot);

long commit_tcs(SGX_tcs *);
#endif
