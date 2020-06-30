#include <sgx_runtime.h>
#include <stddef.h>
#include <shim_table.h>
#include <sys/mman.h>
#include <enclave_mman.h>
#include <sgx.h>

#include <sgx_signal.h>
#include <sgx_thread.h>
/*
 * In enclaves a thread's identity is its tcs
 * here we map the tcs to asorted stackpointers
 */

/* temporary stack to work on while we find the threads identity */
void *out_err_hand;
/*
 * These members are accessed from assembly code, and so should be changed only
 * as a last resort
 */
_Static_assert(offsetof(struct thread, rsp) == 0,
        "struct thread.rsp does not meet offset req");
_Static_assert(offsetof(struct thread, rbp) == 8,
        "struct thread.rbp does not meet offset req");
_Static_assert(offsetof(struct thread, tcs) == 16,
        "struct thread.tcs  does not meet offset req");
_Static_assert(offsetof(struct thread, tls_size) == 24,
        "struct thread.tls_size  does not meet offset req");
_Static_assert(offsetof(struct thread, out_rsp) == 32,
        "struct thread.out_rsp  does not meet offset req");
_Static_assert(offsetof(struct thread, out_rbp) == 40,
        "struct thread.out_rbp  does not meet offset req");
_Static_assert(offsetof(struct thread, sigaltstack) == 56,
        "struct thread.sigaltstack  does not meet offset req");

void *untrusted_syscall_handler;
void *untrusted_error_handler;
void *untrusted_clone_handler;
void *syscall_ret;
uint64_t __tls_size;
uint64_t _tls_size;


void _start(void);
static const void *entry_ptr = _start;

static
SGX_ssa_frame *get_ssa(struct thread *tcs, uint32_t cssa)
{
    return (void*)(tcs->ossa + enclave_params.base) + ((cssa - 1) * SSA_SIZE);
}


void check_exception(uint32_t cssa, struct thread *thread, uintptr_t ret_rip)
{
    if (cssa && (!thread->do_signal)) {
        long ret;
        SGX_ssa_frame *ssa;
        uintptr_t rsp = thread->rsp, rbp = thread->rbp;

        thread->do_signal = 1;
        ssa = get_ssa(thread, cssa);
        ret = dispatch_handler(ssa);
        thread->do_signal = 0;

        thread->rsp = rsp;
        thread->rbp = rbp;
        enclave_exception_exit(ret, ret_rip, ret_rip);
    }
}

static
SGX_tcs *alloc_tcs(unsigned long sp)
{
    SGX_tcs *tcs;
    unsigned long tcs_addr;
    struct thread *thread;
    sgx_lock(&mm_lock);
    tcs_addr = allocate_region(PAGE_SIZE + PAGE_SIZE + (SSA_SIZE * TCS_NSSA),
                               PROT_READ|PROT_WRITE, true);
    sgx_unlock(&mm_lock);

    tcs = (void *)tcs_addr;
    thread = (void *)tcs_addr + PAGE_SIZE;
    thread->rsp = sp;
    thread->rbp = sp;
    thread->tcs = tcs;
    thread->ossa = (tcs_addr + PAGE_SIZE + PAGE_SIZE) - enclave_params.base;
    thread->sigaltstack = 0;
    tcs->ossa = thread->ossa;
    tcs->cssa = 0;
    tcs->nssa = TCS_NSSA;
    return tcs;
}

SGX_tcs *alloc_new_thread(unsigned long sp, unsigned long tls) {
    SGX_tcs *tcs = alloc_tcs(sp);
    if (!tcs) {
        return NULL;
    }
    tcs->oentry = (unsigned long)entry_ptr - enclave_params.base;
    tcs->ofsbase = tls - enclave_params.base;

    /* call out to the OS and change the pagetype of tcs */
    if (commit_tcs(tcs))
        DIE("failed to commit tcs");
    return tcs;
}
