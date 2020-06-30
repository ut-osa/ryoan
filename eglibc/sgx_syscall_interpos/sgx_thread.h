#ifndef _SGX_THREAD_H_
#define _SGX_THREAD_H_

struct thread {
    uint64_t rsp;
    uint64_t rbp;
    void *tcs;
    uint64_t tls_size;
    uint64_t out_rsp;
    uint64_t out_rbp;
    uint64_t ossa;
    uint64_t sigaltstack;
    int      ss_flags;
    int      ss_size;
    int      do_signal;
};

/* from syscallas.S */
void enclave_exception_exit(long ret_val, uintptr_t rip, uintptr_t aep);
struct thread *get_thread_struct(void);

#endif
