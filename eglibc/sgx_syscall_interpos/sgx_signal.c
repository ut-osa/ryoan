#include <sgx_signal.h>
#include <shim_table.h> //for DIE
#include <sgx_runtime.h>
#include <errno.h>
#include <syscall_shim.h> //BUFFUL
#include <sgx_thread.h>

/* SGX reg numbers */
#define R_RAX 0
#define R_RCX 1
#define R_RDX 2
#define R_RBX 3
#define R_RSP 4
#define R_RBP 5
#define R_RSI 6
#define R_RDI 7
/* R8 - R15 likeyou would expect */

/* Def for the opaque mcontext_t */
/* XXX Only works on x86_64!!!! */
struct reg_context {
    uint64_t r8;
    uint64_t r9;
    uint64_t r10;
    uint64_t r11;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;
    uint64_t rdi;
    uint64_t rsi;
    uint64_t rbp;
    uint64_t rbx;
    uint64_t rdx;
    uint64_t rax;
    uint64_t rcx;
    uint64_t rsp;
    uint64_t rip;
    uint64_t rflags;         /* RFLAGS */
    uint16_t cs;
    uint16_t gs;
    uint16_t fs;
    uint16_t __pad0;
    uint64_t err;
    uint64_t trapno;
    uint64_t oldmask;
    uint64_t cr2;
    uint64_t fpstate; /* pointer */
    uint64_t reserved1[8];
};

static int fatal_signal (int sig)
{
    switch (sig) {
    case SIGCHLD:
    case SIGURG:
    case SIGWINCH:
        /* Ignored by default.  */
        return 0;
    case SIGCONT:
    case SIGSTOP:
    case SIGTSTP:
    case SIGTTIN:
    case SIGTTOU:
        /* Job control signals.  */
        return 0;
    default:
        return 1;
    }
}
static struct kernel_sigaction sigact_table[NSIG];

static inline
void gpr_sgx_to_reg_context(struct reg_context *ctx, const gpr_sgx *gpr)
{
    ctx->r8  = gpr->regs[8];
    ctx->r9  = gpr->regs[9];
    ctx->r10 = gpr->regs[10];
    ctx->r11 = gpr->regs[11];
    ctx->r12 = gpr->regs[12];
    ctx->r13 = gpr->regs[13];
    ctx->r14 = gpr->regs[14];
    ctx->r15 = gpr->regs[15];
    ctx->rdi = gpr->regs[R_RDI];
    ctx->rsi = gpr->regs[R_RSI];
    ctx->rbp = gpr->regs[R_RBP];
    ctx->rbx = gpr->regs[R_RBX];
    ctx->rdx = gpr->regs[R_RDX];
    ctx->rax = gpr->regs[R_RAX];
    ctx->rcx = gpr->regs[R_RCX];
    ctx->rsp = gpr->regs[R_RSP];

    ctx->rip = gpr->rip;
    ctx->rflags = gpr->rflags;
    /* TODO ctx->fs
    uint64_t err;
    uint64_t trapno;
    uint64_t oldmask;
    uint64_t cr2;
    uint64_t fpstate;
    uint64_t reserved1[8];
    */
}

static inline
void reg_context_to_gpr_sgx(gpr_sgx *gpr, const struct reg_context *ctx)
{
    gpr->regs[8] = ctx->r8;
    gpr->regs[9] = ctx->r9;
    gpr->regs[10] = ctx->r10;
    gpr->regs[11] = ctx->r11;
    gpr->regs[12] = ctx->r12;
    gpr->regs[13] = ctx->r13;
    gpr->regs[14] = ctx->r14;
    gpr->regs[15] = ctx->r15;
    gpr->regs[R_RDI] = ctx->rdi;
    gpr->regs[R_RSI] = ctx->rsi;
    gpr->regs[R_RBP] = ctx->rbp;
    gpr->regs[R_RBX] = ctx->rbx;
    gpr->regs[R_RDX] = ctx->rdx;
    gpr->regs[R_RAX] = ctx->rax;
    gpr->regs[R_RCX] = ctx->rcx;
    gpr->regs[R_RSP] = ctx->rsp;

    gpr->rip = ctx->rip;
    gpr->rflags = ctx->rflags;
    /* TODO ctx->fs
    uint64_t err;
    uint64_t trapno;
    uint64_t oldmask;
    uint64_t cr2;
    uint64_t fpstate;
    uint64_t reserved1[8];
    */
}
#define GET_REG_CONTEXT(c) \
    ((struct reg_context *)(&((ucontext_t *)c)->uc_mcontext))

static
long run_handler(int sig, SGX_ssa_frame *ssa)
{
    struct kernel_sigaction *k;
    struct reg_context *ctx;
    siginfo_t info;
    ucontext_t ucontext;
    void (*real_handler)(int, siginfo_t *, void *);

    ctx = GET_REG_CONTEXT(&ucontext);

    /* populate reg_context */
    gpr_sgx_to_reg_context(ctx, &ssa->gprsgx);

    /* populate info */
    info.si_signo = sig;
    switch(sig) {
        case SIGSEGV:
            info.si_addr = (void *)ssa->misc.exitinfo.maddr;
            /* XXX Should check error code */
            info.si_code = SEGV_ACCERR;
            break;
        default:
            DIE("UNEXPECTED SIGNAL TO HANDLE");
    }

    /* Run the thing */
    k = &sigact_table[sig - 1];
    if (k->k_sa_handler) {
        real_handler = (void *)k->k_sa_handler;
        real_handler(sig, &info, &ucontext);
        reg_context_to_gpr_sgx(&ssa->gprsgx, ctx);
        return 0;
    }
    return -1;
}

#define EXCP_PF 14
long dispatch_handler(SGX_ssa_frame *ssa)
{
    long ret = -1;
    switch (ssa->gprsgx.exitinfo) {
        case EXCP_PF:
            ret = run_handler(SIGSEGV, ssa);
            break;
        default:
            DIE("UNEXPECTED EXIT REASON");
    }
    return ret;
}

int do_sigaction(int sig, const struct kernel_sigaction *act,
        struct kernel_sigaction *oact, size_t sigsize)
{
    static sgx_mutex sigact_lock;
    struct kernel_sigaction *k;
    struct kernel_sigaction *act1;
    int ret = 0;

    if (sig < 1 || sig > NSIG || sig == SIGKILL || sig == SIGSTOP)
        return -EINVAL;

    k = &sigact_table[sig - 1];
    if (oact) {
        memcpy(oact, k, sizeof(struct kernel_sigaction));
    }
    if (act) {
        sgx_lock(&sigact_lock);
        memcpy(k, act, sizeof(struct kernel_sigaction));

        /* we update the host linux signal state */
        if (sig != SIGSEGV && sig != SIGBUS) {
            act1 = get_syscall_buffer(sizeof(struct kernel_sigaction));
            if (!act1) return -EBUFFUL;
            //sigfillset(&act1->sa_mask);
            act1->sa_mask = k->sa_mask;
            act1->sa_flags = SA_SIGINFO;
            if (k->sa_flags & SA_RESTART)
                act1->sa_flags |= SA_RESTART;
            /* NOTE: it is important to update the host kernel signal
               ignore state to avoid getting unexpected interrupted
               syscalls */
            if (k->k_sa_handler == SIG_IGN) {
                act1->k_sa_handler = (void *)SIG_IGN;
            } else if (k->k_sa_handler == SIG_DFL) {
                if (fatal_signal (sig))
                    act1->k_sa_handler = enclave_params.signal_handler;
                else
                    act1->k_sa_handler = (void *)SIG_DFL;
            } else {
                act1->k_sa_handler = enclave_params.signal_handler;
            }
            ret = PASS_THROUGH_SYSCALL(rt_sigaction, 4,  sig, act1, NULL,
                    sigsize);
            free_syscall_buffer(act1);
        }
        sgx_unlock(&sigact_lock);
    }
    return ret;
}

long do_sigaltstack(const stack_t *ss, stack_t *oss)
{
    struct thread *thread = get_thread_struct();
    if (oss) {
        oss->ss_sp = (void *)thread->sigaltstack;
        oss->ss_flags = thread->ss_flags;
        oss->ss_size = thread->ss_size;
    }
    if (ss) {
        if (thread->ss_flags == SS_DISABLE) {
            thread->sigaltstack = 0;
            thread->ss_flags = 0;
            thread->ss_size = 0;
        } else {
            thread->sigaltstack = ((uintptr_t)ss->ss_sp + ss->ss_size) & ~0xFUL;
            thread->ss_flags = ss->ss_flags;
            thread->ss_size = ss->ss_size;
        }
    }
    return 0;
}

long do_sigreturn(void)
{
    DIE("SIGRETURN should not be called\n");
    return 0;
}
