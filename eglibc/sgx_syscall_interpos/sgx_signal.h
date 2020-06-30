#ifndef _SGX_SIGNAL_H_
#define _SGX_SIGNAL_H_
#include <signal.h>
#include <stdint.h>
#include <sgx_runtime.h>

/* This is the sigaction structure from the Linux 2.1.68 kernel.  */
struct kernel_sigaction {
	__sighandler_t k_sa_handler;
	unsigned long sa_flags;
	void (*sa_restorer) (void);
	sigset_t sa_mask;
};

int do_sigaction(int sig, const struct kernel_sigaction *act,
        struct kernel_sigaction *oact, size_t sigsize);

long dispatch_handler(SGX_ssa_frame *ssa);

long do_sigaltstack(const stack_t *ss, stack_t *oss);
#endif
