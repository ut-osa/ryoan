#ifndef _SGX_RUNTIME_H_
#define _SGX_RUNTIME_H_
#include<stdint.h>
#include<stdbool.h>
#include<sys/user.h>


#define MAP_UNTRUST 0x100UL
/*
 * Hack so that we can switch syscall behavior depending on weather or not we
 * are in an enclave
 */
#define SGX_KEY 0x11223344
#define STR(x) _STR(x)
#define _STR(x) #x
#define DIE(msg) HCF("FATAL [" STR(__FILE__)":" STR(__LINE__)"]: " msg "\n");
#define BAD(msg) WARN("BAD STUFF [" STR(__FILE__)":" STR(__LINE__)"]: "\
        msg "\n");
extern bool in_sgx;
void HCF(const char*); // Halt and catch fire
void WARN(const char*);// just complain loudly

void sgx_yield(void);
typedef volatile int sgx_mutex;
static inline void sgx_lock(sgx_mutex *m)
{
    while (__sync_lock_test_and_set(m, 1)) {
    }
}
static inline void sgx_unlock(sgx_mutex *m)
{
    __sync_synchronize(); // Memory barrier.
    *m = 0;
}

typedef struct gpr_sgx {
    uint64_t regs[16];
    uint64_t rflags;
    uint64_t rip;
    uint64_t ursp;
    uint64_t urbp;
    uint32_t exitinfo;
} gpr_sgx;

struct misc_region {
    /* TBD (that's what the manual says) */
    struct {
        uint64_t maddr;
        uint32_t errcd;
    } exitinfo;
};

typedef struct SGX_ssa_frame {
    struct misc_region misc;
    gpr_sgx gprsgx;
} SGX_ssa_frame;


/*
 * Structure which describes SGX state which is not in our control, and as such
 * cannot be known until runtime.
 */
struct sgx_man_info {
    void *syscall_buffer;
    uint64_t syscall_buffer_size;
    uint32_t *futex_pool;
    uint32_t n_futexes;
    uint64_t brk;
    uint64_t base;
    uint64_t size;
    uint64_t mmap_line;
    uint64_t main_fs;
    void *syscall_handler;
    void *error_handler;
    void *clone_handler;
    void *signal_handler;
};
extern struct sgx_man_info enclave_params;
#define get_fs()\
({\
    void *__ret = (void *)enclave_params.main_fs;\
    __ret;\
 })
#endif
