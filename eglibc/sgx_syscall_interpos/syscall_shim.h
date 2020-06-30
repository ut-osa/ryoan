/* Copyright (C) 2001-2015 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <http://www.gnu.org/licenses/>.  */

#ifndef _SYSCALL_MACROS_H
#define _SYSCALL_MACROS_H
#include <sys/uio.h>
#include <sys/socket.h>
#include <sgx_runtime.h>
#include <string.h>
#include <asm/unistd_64.h>
#include <shim_table.h>
#include "usec_wait.h"

#define EEXIT_EENTER_DELAY 6250UL

#define ROUND_UP(addr) ((addr + PG_SIZE - 1) & ~(PG_SIZE - 1))
#define ROUND_DOWN(addr) (addr & ~(PG_SIZE - 1))

#define SYSCALL_SHIM_DEF_0(name)                                              \
    long __shim_##name(void) {

#define SYSCALL_SHIM_DEF_1(name, t1, p1)                                      \
    long __shim_##name(long __a) {                                            \
        t1 p1 = (t1) __a;

#define SYSCALL_SHIM_DEF_2(name, t1, p1, t2, p2)                              \
    long __shim_##name(long __a, long __b) {                                  \
        t1 p1 = (t1) __a;                                                     \
        t2 p2 = (t2) __b;

#define SYSCALL_SHIM_DEF_3(name, t1, p1, t2, p2, t3, p3)                      \
    long __shim_##name(long __a, long __b, long __c) {                        \
        t1 p1 = (t1) __a;                                                     \
        t2 p2 = (t2) __b;                                                     \
        t3 p3 = (t3) __c;

#define SYSCALL_SHIM_DEF_4(name, t1, p1, t2, p2, t3, p3, t4, p4)              \
    long __shim_##name(long __a, long __b, long __c, long __d) {              \
        t1 p1 = (t1) __a;                                                     \
        t2 p2 = (t2) __b;                                                     \
        t3 p3 = (t3) __c;                                                     \
        t4 p4 = (t4) __d;

#define SYSCALL_SHIM_DEF_5(name, t1, p1, t2, p2, t3, p3, t4, p4, t5, p5)      \
    long __shim_##name(long __a, long __b, long __c, long __d, long __e) {    \
        t1 p1 = (t1) __a;                                                     \
        t2 p2 = (t2) __b;                                                     \
        t3 p3 = (t3) __c;                                                     \
        t4 p4 = (t4) __d;                                                     \
        t5 p5 = (t5) __e;

#define SYSCALL_SHIM_DEF_6(name, t1, p1, t2, p2, t3, p3, t4, p4, t5, p5,      \
        t6, p6)                                                               \
    long __shim_##name(long __a, long __b, long __c, long __d, long __e,      \
            long __f) {                                                       \
        t1 p1 = (t1) __a;                                                     \
        t2 p2 = (t2) __b;                                                     \
        t3 p3 = (t3) __c;                                                     \
        t4 p4 = (t4) __d;                                                     \
        t5 p5 = (t5) __e;                                                     \
        t6 p6 = (t6) __f;

#define SYSCALL_SHIM_END }

/* For Linux we can use the system call table in the header file
   /usr/include/asm/unistd.h
   of the kernel.  But these symbols do not follow the SYS_* syntax
   so we have to redefine the `SYS_ify' macro here.  */
#undef SYS_ify
#define SYS_ify(syscall_name)   __NR_##syscall_name

/* This is a kludge to make syscalls.list find these under the names
   pread and pwrite, since some kernel headers define those names
   and some define the *64 names for the same system calls.  */
#if !defined __NR_pread && defined __NR_pread64
# define __NR_pread __NR_pread64
#endif
#if !defined __NR_pwrite && defined __NR_pwrite64
# define __NR_pwrite __NR_pwrite64
#endif

/* This is to help the old kernel headers where __NR_semtimedop is not
   available.  */
#ifndef __NR_semtimedop
# define __NR_semtimedop 220
#endif

#ifndef __NR_getrandom
# define __NR_getrandom 318
#endif
/* XXX Don't want to deal with multiple unistd right now so I'll just leave
   this here... */
#define __NR_enclaveinit 323
#define __NR_enclave_aug 324
#define __NR_enclave_release 325
#define __NR_enclave_drop 326
#define __NR_enclave_make_tcs 327
#define __NR_lockmprotect 328
#define __NR_nodelaymprotect 329
struct sched_attr {
    unsigned int size;
    unsigned int sched_policy;
    unsigned long sched_flags;
    int sched_nice;
    unsigned int sched_priority;
    unsigned long sched_runtime;
    unsigned long sched_deadlock;
    unsigned long sched_period;
};

/* Define a macro which expands inline into the wrapper code for a system
   call.  */
# define PASS_THROUGH_SYSCALL(name, nr_args...) \
  ({                                 \
    unsigned long int resultvar = SGX_INTERNAL_SYSCALL (name, , ##nr_args);    \
    (long int) resultvar; })

# define PASS_THROUGH_SYSCALL_NODELAY(name, nr_args...) \
  ({                                 \
    unsigned long int resultvar = SGX_INTERNAL_SYSCALL_NODELAY (name, , ##nr_args);    \
    (long int) resultvar; })
# define SGX_INTERNAL_SYSCALL_NCS_NORM(name, err, nr, args...) \
  ({                                 \
    unsigned long int resultvar;                     \
    LOAD_ARGS_##nr (args)                        \
    LOAD_REGS_##nr                           \
    asm volatile (                           \
    "movq syscall_pass@GOTPCREL(%%rip), %%rcx\n\t"                  \
    "call *%%rcx\n\t"                           \
    : "=a" (resultvar)                           \
    : "0" (name) ASM_ARGS_##nr : "memory", "cc", "r11", "cx");         \
    (long int) resultvar; })

# define SGX_INTERNAL_SYSCALL_NCS_ENCL(name, err, nr, args...) \
  ({                                 \
    unsigned long int resultvar;                     \
    LOAD_ARGS_##nr (args)                        \
    LOAD_REGS_##nr                           \
    asm volatile (                           \
    "subq $128, %%rsp\n\t"\
    "movq enclave_syscall_exit@GOTPCREL(%%rip), %%rcx\n\t"  \
    "call *%%rcx\n\t"                           \
    "addq $128, %%rsp\n\t"\
    : "=a" (resultvar)                           \
    : "0" (name) ASM_ARGS_##nr : "memory", "cc", "rcx",\
            "r11", "r12", "r13", "r14", "r15", "rbx"); \
    (long int) resultvar; })

# define SGX_INTERNAL_SYSCALL(name, err, nr_args...) \
  ({\
    unsigned long int ___res; \
    if (in_sgx) { \
        ___res = SGX_INTERNAL_SYSCALL_NCS_ENCL (__NR_##name, err, ##nr_args); \
    } else { \
        eenter_wait(); \
        ___res = SGX_INTERNAL_SYSCALL_NCS_NORM (__NR_##name, err, ##nr_args); \
    } \
    (long int) ___res; })

# define SGX_INTERNAL_SYSCALL_NODELAY(name, err, nr_args...) \
  ({\
    unsigned long int ___res; \
    if (in_sgx) { \
        ___res = SGX_INTERNAL_SYSCALL_NCS_ENCL (__NR_##name, err, ##nr_args); \
    } else { \
        ___res = SGX_INTERNAL_SYSCALL_NCS_NORM (__NR_##name, err, ##nr_args); \
    } \
    (long int) ___res; })
# define SGX_INTERNAL_SYSCALL_NCS_ASM SGX_INTERNAL_SYSCALL

# define LOAD_ARGS_0()
# define LOAD_REGS_0
# define ASM_ARGS_0

# define LOAD_ARGS_TYPES_1(t1, a1)                  \
  t1 __arg1 = (t1) (a1);                     \
  LOAD_ARGS_0 ()
# define LOAD_REGS_TYPES_1(t1, a1)                  \
  register t1 _a1 asm ("rdi") = __arg1;                  \
  LOAD_REGS_0
# define ASM_ARGS_1   ASM_ARGS_0, "r" (_a1)
# define LOAD_ARGS_1(a1)                     \
  LOAD_ARGS_TYPES_1 (long int, a1)
# define LOAD_REGS_1                        \
  LOAD_REGS_TYPES_1 (long int, a1)

# define LOAD_ARGS_TYPES_2(t1, a1, t2, a2)               \
  t2 __arg2 = (t2) (a2);                     \
  LOAD_ARGS_TYPES_1 (t1, a1)
# define LOAD_REGS_TYPES_2(t1, a1, t2, a2)               \
  register t2 _a2 asm ("rsi") = __arg2;                  \
  LOAD_REGS_TYPES_1(t1, a1)
# define ASM_ARGS_2   ASM_ARGS_1, "r" (_a2)
# define LOAD_ARGS_2(a1, a2)                     \
  LOAD_ARGS_TYPES_2 (long int, a1, long int, a2)
# define LOAD_REGS_2                        \
  LOAD_REGS_TYPES_2 (long int, a1, long int, a2)

# define LOAD_ARGS_TYPES_3(t1, a1, t2, a2, t3, a3)            \
  t3 __arg3 = (t3) (a3);                     \
  LOAD_ARGS_TYPES_2 (t1, a1, t2, a2)
# define LOAD_REGS_TYPES_3(t1, a1, t2, a2, t3, a3)            \
  register t3 _a3 asm ("rdx") = __arg3;                  \
  LOAD_REGS_TYPES_2(t1, a1, t2, a2)
# define ASM_ARGS_3   ASM_ARGS_2, "r" (_a3)
# define LOAD_ARGS_3(a1, a2, a3)                  \
  LOAD_ARGS_TYPES_3 (long int, a1, long int, a2, long int, a3)
# define LOAD_REGS_3                        \
  LOAD_REGS_TYPES_3 (long int, a1, long int, a2, long int, a3)

# define LOAD_ARGS_TYPES_4(t1, a1, t2, a2, t3, a3, t4, a4)         \
  t4 __arg4 = (t4) (a4);                     \
  LOAD_ARGS_TYPES_3 (t1, a1, t2, a2, t3, a3)
# define LOAD_REGS_TYPES_4(t1, a1, t2, a2, t3, a3, t4, a4)         \
  register t4 _a4 asm ("r10") = __arg4;                  \
  LOAD_REGS_TYPES_3(t1, a2, t2, a2, t3, a3)
# define ASM_ARGS_4   ASM_ARGS_3, "r" (_a4)
# define LOAD_ARGS_4(a1, a2, a3, a4)                  \
  LOAD_ARGS_TYPES_4 (long int, a1, long int, a2, long int, a3,         \
           long int, a4)
# define LOAD_REGS_4                        \
  LOAD_REGS_TYPES_4 (long int, a1, long int, a2, long int, a3,         \
           long int, a4)

# define LOAD_ARGS_TYPES_5(t1, a1, t2, a2, t3, a3, t4, a4, t5, a5)      \
  t5 __arg5 = (t5) (a5);                     \
  LOAD_ARGS_TYPES_4 (t1, a1, t2, a2, t3, a3, t4, a4)
# define LOAD_REGS_TYPES_5(t1, a1, t2, a2, t3, a3, t4, a4, t5, a5)      \
  register t5 _a5 asm ("r8") = __arg5;                  \
  LOAD_REGS_TYPES_4 (t1, a1, t2, a2, t3, a3, t4, a4)
# define ASM_ARGS_5   ASM_ARGS_4, "r" (_a5)
# define LOAD_ARGS_5(a1, a2, a3, a4, a5)               \
  LOAD_ARGS_TYPES_5 (long int, a1, long int, a2, long int, a3,         \
           long int, a4, long int, a5)
# define LOAD_REGS_5                        \
  LOAD_REGS_TYPES_5 (long int, a1, long int, a2, long int, a3,         \
           long int, a4, long int, a5)

# define LOAD_ARGS_TYPES_6(t1, a1, t2, a2, t3, a3, t4, a4, t5, a5, t6, a6) \
  t6 __arg6 = (t6) (a6);                     \
  LOAD_ARGS_TYPES_5 (t1, a1, t2, a2, t3, a3, t4, a4, t5, a5)
# define LOAD_REGS_TYPES_6(t1, a1, t2, a2, t3, a3, t4, a4, t5, a5, t6, a6) \
  register t6 _a6 asm ("r9") = __arg6;                  \
  LOAD_REGS_TYPES_5 (t1, a1, t2, a2, t3, a3, t4, a4, t5, a5)
# define ASM_ARGS_6   ASM_ARGS_5, "r" (_a6)
# define LOAD_ARGS_6(a1, a2, a3, a4, a5, a6)               \
  LOAD_ARGS_TYPES_6 (long int, a1, long int, a2, long int, a3,         \
           long int, a4, long int, a5, long int, a6)
# define LOAD_REGS_6                        \
  LOAD_REGS_TYPES_6 (long int, a1, long int, a2, long int, a3,         \
           long int, a4, long int, a5, long int, a6)

#define EBUFFUL ENOMEM;
#define GET_BUFFER_IF(sz, val) (val ? get_syscall_buffer(sz) : NULL)


#define INT_NULL ((unsigned long *)0)
/* This is a macro and not an inlined function because we want to support
 * variable sized integer sizes */
#define cpy_alloc_buffer(__buf, __buflen, __retlen) \
({                                                  \
    void *__ret = GET_BUFFER_IF(__buflen, __buf);   \
    if (__ret) {                                    \
        memcpy(__ret, __buf, __buflen);             \
        if (__retlen) *__retlen = __buflen;         \
    }                                               \
    __ret;                                          \
 })

#define UNEXP_NULL(_new, _old, field) (!(_new)->field && (_old)->field)
/* assumes all gather/scatter locations are the same size*/
static inline
void cpy_iovec(struct iovec *dst, const struct iovec *src, size_t count)
{
    size_t i;
    if (!src || !dst) return;

    for (i = 0; i < count; ++i) {
        memcpy(dst[i].iov_base, src[i].iov_base, dst[i].iov_len);
    }
}
static inline
void free_iovec(struct iovec *vec, size_t len)
{
    size_t i;
    for (i = 0; i < len; ++i)
        free_syscall_buffer(vec[i].iov_base);
    free_syscall_buffer(vec);
}

static inline
struct iovec *alloc_iovec(const struct iovec *vec, size_t veclen)
{
    size_t i;
    struct iovec *ret = get_syscall_buffer(sizeof(struct iovec) * veclen);
    if (!ret) return NULL;
    memset(ret, 0, sizeof(struct iovec) * veclen);
    for (i = 0; i < veclen; ++i) {
        ret[i].iov_base = get_syscall_buffer(vec[i].iov_len);
        ret[i].iov_len = vec[i].iov_len;
        if (UNEXP_NULL(ret + i, vec + i, iov_base)) {
            free_iovec(ret, veclen);
            return NULL;
        }
    }
    return ret;
}


static inline
void free_msghdr(struct msghdr *msg)
{
    free_syscall_buffer(msg->msg_name);
    free_syscall_buffer(msg->msg_control);
    free_iovec(msg->msg_iov, msg->msg_iovlen);
    free_syscall_buffer(msg);
}


static inline
struct msghdr *alloc_msghdr(const struct msghdr *msg)
{
    struct msghdr *ret = GET_BUFFER_IF(sizeof(struct msghdr), msg);
    if (!ret) return NULL;

    ret->msg_name = get_syscall_buffer(msg->msg_namelen);
    ret->msg_namelen = msg->msg_namelen;
    ret->msg_control = get_syscall_buffer(msg->msg_controllen);
    ret->msg_controllen = msg->msg_controllen;
    ret->msg_iov = alloc_iovec(msg->msg_iov, msg->msg_iovlen);
    ret->msg_iovlen = msg->msg_iovlen;

    if (UNEXP_NULL(ret, msg, msg_name) || UNEXP_NULL(ret, msg, msg_control) ||
            UNEXP_NULL(ret, msg, msg_iov))
    {
        free_msghdr(ret);
        ret = NULL;
    }
    return ret;
}


/*assumes that all internal buffers are the same size*/
static inline
void cpy_msghdr(struct msghdr *dst, const struct msghdr *src)
{
    if (!src || !dst) return;

    dst->msg_flags = src->msg_flags;
    memcpy(dst->msg_name, src->msg_name, dst->msg_namelen);
    memcpy(dst->msg_control, src->msg_control, dst->msg_controllen);
    cpy_iovec(dst->msg_iov, src->msg_iov, dst->msg_iovlen);
}

long do_clone_exit(unsigned long flags, int *ptid, int *ctid, SGX_tcs *);
#endif /* _SYSCALL_MACROS */
