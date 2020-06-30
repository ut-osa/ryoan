#include <shim_table.h>
#include <sysdep.h>
#include <sgx_macros.h>

#define CALC_THREAD_BLOCK(block)\
    movq _tls_size@GOTPCREL(%rip), block;\
    movq (block), block;\
    subq $8, block;\
    movq %fs:(block), block;

#define THREAD_SWAP_STATE(scr, tcs)\
    CALC_THREAD_BLOCK(scr) \
    movq %rsp, 0(scr);\
    movq %rbp, 8(scr);\
    movq 16(scr), tcs;\
    movq 32(scr), %rsp;\
    movq 40(scr), %rbp;

#define DO_EEXIT(syscall_hand)\
    movq syscall_hand@GOTPCREL(%rip), %r11;\
    movq (%r11), %rbx;\
    movq untrusted_error_handler@GOTPCREL(%rip), %r11;\
    movq (%r11), %rcx;\
    movq %rax, %r11;\
    movq $EEXIT, %rax;\
    enclu


    .global enclave_exception_exit
    .type enclave_exception_exit, @function
enclave_exception_exit:
    CALC_THREAD_BLOCK(%rbx)
    movq 32(%rbx), %rsp
    movq 40(%rbx), %rbp
    movq %rdi, %r11
    movq %rsi, %rbx
    movq %rdx, %rcx
    movq $EEXIT, %rax
    enclu

    .global get_thread_struct
    .type get_thread_struct, @function
get_thread_struct:
    CALC_THREAD_BLOCK(%rax)
    ret


/* assumes syscall number is stored in r11 */
    .global enclave_syscall_exit
    .type enclave_syscall_exit, @function
enclave_syscall_exit:
    /* Save these */
    pushq %rdi
    pushq %rsi
    pushq %rdx
    pushq %r10
    pushq %r8
    pushq %r9

    movq __SYSCALL_RET@GOTPCREL(%rip), %r11 //save ret addr
    pushq %r11

    THREAD_SWAP_STATE(%r11, %r12)

    DO_EEXIT(untrusted_syscall_handler)

__SYSCALL_RET:
    movq %r11, %rax
    /* and restore...*/
    popq %r9
    popq %r8
    popq %r10
    popq %rdx
    popq %rsi
    popq %rdi
    ret

    .global do_clone_exit
    .type do_clone_exit, @function
do_clone_exit:
    /* Save these */
    pushq %rdi
    pushq %rsi
    pushq %rdx
    pushq %r10
    pushq %r8
    pushq %r9
    pushq %rbx
    pushq %rcx

    pushq %r11
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15

    movq %rcx, %r10

    movq __CLONE_RET@GOTPCREL(%rip), %r11 //save ret addr
    pushq %r11

    THREAD_SWAP_STATE(%r11, %r12)
    DO_EEXIT(untrusted_clone_handler)

__CLONE_RET:
    movq %r11, %rax
    /* and restore...*/
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %r11

    popq %rcx
    popq %rbx
    popq %r9
    popq %r8
    popq %r10
    popq %rdx
    popq %rsi
    popq %rdi
    ret

    .global syscall_pass
    .type syscall_pass, @function
syscall_pass:
    cfi_startproc
    syscall
    ret
    cfi_endproc
    .size syscall_pass,.-syscall_pass

    .global __syscall_pass
    .type __syscall_pass, @function
__syscall_pass:
    cfi_startproc
    syscall
    ret
    cfi_endproc
    .size __syscall_pass,.-__syscall_pass

/* soft failure */
    .global syscall_fail_soft
    .type syscall_fail_soft,@function
syscall_fail_soft:
    cfi_startproc
    movq $-38, %rax
    ret
    cfi_endproc

    .global syscalldb
    .type syscalldb, @function
syscalldb:
    cfi_startproc

    # avoid red zone for previous rsp
    subq $128, %rsp
    cfi_adjust_cfa_offset(128)

    # DEP 7/9/12: Push a stack pointer so clone can find the return address
    pushq %rbp
    cfi_adjust_cfa_offset(8)
    movq %rsp, %rbp
    cmp $SHIM_NSYSCALLS , %rax
    jae isundef
    movq shim_table@GOTPCREL(%rip), %r11
    movq (%r11,%rax,8), %r11
    cmp $0, %r11
    je isundef

isdef:
    pushq %rbx
    pushq %rdi
    pushq %rsi
    pushq %rdx
    pushq %r8
    pushq %r9
    pushq %r10
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15
    cfi_adjust_cfa_offset(88)

    movq %r10, %rcx /* since we're calling a regular function now */
    call *%r11

    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %r10
    popq %r9
    popq %r8
    popq %rdx
    popq %rsi
    popq %rdi
    popq %rbx
    cfi_adjust_cfa_offset(-88)

    jmp ret

isundef:
    /*movq $-38, %rax*/
    movq %rax, %rdi
    movq NO_SYSCALL_FAIL@GOTPCREL(%rip), %rax
    jmp *%rax

ret:
    popq %rbp
    cfi_adjust_cfa_offset(-8)
    addq $128, %rsp
    cfi_adjust_cfa_offset(-128)
    retq

    cfi_endproc
    .size syscalldb, .-syscalldb

