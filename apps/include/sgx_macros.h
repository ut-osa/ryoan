#ifndef _SGX_MACROS_H
#define _SGX_MACROS_H

#ifdef __cplusplus
extern "C" {
#endif

/* enable this if your compiler doesn't support them;
 * disabled by default becuase these instructions are legitimately
 * used sometimes
 */
//#define USE_PSEUDO_OPS 1

#define align64 __attribute__((aligned(64)))

#define PG_SIZE 4096
#define SSA_SIZE PG_SIZE
/*** System Leaves ***/
#define ECREATE 0x00 /* pg 75  */
#define EADD 0x01    /* pg 65  */
#define EINIT 0x02   /* pg 85  */
#define EREMOVE 0x03 /* pg 106 */
#define EDBGRD 0x04  /* pg 79  */
#define EDBGWR 0x05  /* pg 82  */
#define EEXTEND 0x06 /* pg 85  */
#define ELDB 0x07    /* pg 95  */
#define ELDU 0x08    /* pg 95  */
#define EBLOCK 0x09  /* pg 72  */
#define EPA 0x0A     /* pg 106 */
#define EWB 0x0B     /* pg 113 */
#define ETRACK 0x0C  /* pg 111 */
#define EAUG 0x0D    /* pg 69  */
#define EMODPR 0x0E  /* pg 100 */
#define EMODT 0x0F   /* pg 103 */

/*** User Leaves ***/
#define EREPORT 0x00     /* pg 148 */
#define EGETKEY 0x01     /* pg 138 */
#define EENTER 0x02      /* pg 127 */
#define ERESUME 0x03     /* pg 152 */
#define EEXIT 0x04       /* pg 135 */
#define EACCEPT 0x05     /* pg 119 */
#define EMODPE 0x06      /* pg 145 */
#define EACCEPTCOPY 0x07 /* pg 123 */

#define LAUNCH_KEY 0
#define PROVISION_KEY 1
#define PROVISION_SEAL_KEY 2
#define REPORT_KEY 3
#define SEAL_KEY 4

#define SGX_REQ_MRENCLAVE 0x1
#define SGX_REQ_MRSIGNER 0x2

#ifdef __ASSEMBLER__
#ifdef USE_PSEUDO_OPS
#define ENCLU(leaf)  \
  movq $leaf, % rax; \
  xchg % rcx, % rcx;
#define ENCLS(leaf)  \
  movq $leaf, % rax; \
  xchg % rbx, % rbx;
#else /*!USE_PSEUDO_OPS*/
#define ENCLU(leaf)  \
  movq $leaf, % rax; \
  enclu
#define ENCLS(leaf)  \
  movq $leaf, % rax; \
  encls
#endif /* USE_PSEUDO_OPS*/
#else  /* !__ASSEMBER__ */
#define LOAD_LEAF(leaf) asm volatile("mov %[loc], %%rax" ::[loc] "i"(leaf) :);

#ifdef USE_PSEUDO_OPS
#define ENCLU(leaf)                  \
  do {                               \
    LOAD_LEAF(leaf)                  \
    asm volatile("xchg %rcx, %rcx"); \
  } while (0);

#define ENCLS(leaf)                  \
  do {                               \
    LOAD_LEAF(leaf)                  \
    asm volatile("xchg %rbx, %rbx"); \
  } while (0);
#else /*!USE_PSEUDO_OPS*/
#define ENCLU(leaf)        \
  do {                     \
    LOAD_LEAF(leaf)        \
    asm volatile("enclu"); \
  } while (0);

#define ENCLS(leaf)        \
  do {                     \
    LOAD_LEAF(leaf)        \
    asm volatile("encls"); \
  } while (0);
#endif /* USE_PSEUDO_OPS*/

#define _RAX "rax"
#define _RBX "rbx"
#define _RCX "rcx"
#define _RDX "rdx"
#define _RSP "rsp"
#define _RBP "rbp"
#define _RSI "rsi"
#define _RDI "rdi"
#define _R10 "r10"
#define _R9 "r9"

#define SET(reg, val, t) asm volatile("mov %0, %%" reg::t(val) : reg)
#define SETI(reg, val) SET(reg, val, "i")
#define SETM(reg, val) SET(reg, val, "m")

#endif
/* error codes */
#define SGX_CHILD_PRESENT 13
#define SGX_ENCLAVE_ACT 14
#define SGX_PAGE_ATTRIBUTES_MISMATCH 19
#define SGX_PAGE_NOT_MODIFIABLE 20

/* canned enclave */
//#define MAX_THREADS 32
#define ENCLAVE_START 0x100000000000
#define ENCLAVE_SIZE 0x1000000000
#define ENCLAVE_END (ENCLAVE_START + ENCLAVE_SIZE)
#define SSA_OFFSET 0xc00000000
#define TCS_OFFSET (SSA_OFFSET - PG_SIZE)
#define TCS_NSSA 5

#define TCS_ADDR ENCLAVE_START
#define LDSO_ADDR 0x80000000
/* hackalicouls hardcoded location which tells us where to find the syscall
   handling routine. */
#define SYSCALLDB_LOC 0x10009000f000
#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _SGX_MACROS_H */
