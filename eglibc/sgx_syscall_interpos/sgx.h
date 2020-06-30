#ifndef _TARGET_I386_SGX_DS_H_
#define _TARGET_I386_SGX_DS_H_

#ifdef __KERNEL__
#include <linux/types.h>
#define __CTX
#else
#include <stdint.h>
#include <openssl/evp.h>
#define __CTX EVP_MD_CTX *ctx;
#endif

#include "sgx_macros.h"
#ifdef __cplusplus
extern "C"
{
#endif

//typedef void *(SGX_excp_func)(void);

#define GEN_LABEL(x, y) x ## y
#define LABEL(x, y) GEN_LABEL(x, y)
#define RESERVE_BYTES(b) uint8_t LABEL(res, __LINE__)[b]

/* ERROR CODES */
#define SGX_INVALID_SIG_STRUCT        1
#define SGX_INVALID_ATTRIBUTE         2
#define SGX_BLSTATE                   3
#define SGX_INVALID_MEASUREMENT       4
#define SGX_NOTBLOCKABLE              5
#define SGX_PG_INVLD                  6
#define SGX_LOCKFAIL                  7
#define SGX_INVALID_SIGNATURE         8
#define SGX_MAC_COMPARE_FAIL          9
#define SGX_PAGE_NOT_BLOCKED         10
#define SGX_NOT_TRACKED              11
#define SGX_VA_SLOT_OCCUPIED         12
#define SGX_CHILD_PRESENT            13
#define SGX_ENCLAVE_ACT              14
#define SGX_ENTRYEPOCH_LOCKED        15
#define SGX_INVALID_EINIT_TOKEN      16
#define SGX_PREV_TRK_INCMPL          17
#define SGX_IS_SECS                  18
#define SGX_PAGE_ATTRIBUTES_MISMATCH 19
#define SGX_PAGE_NOT_MODIFIABLE      20
#define SGX_INVALID_CPUSVN           32
#define SGX_INVALID_ISVSVN           64
#define SGX_UNMASKED_EVENT          128
#define SGX_INVALID_KEYNAME         256

/* MRENCLAVE ops */
#define ACTION_ECREATE 0x0045544145524345
#define ACTION_EADD    0x0000000044444145
#define ACTION_EEXTEND 0x00444e4554584545




typedef struct SGX_identity_t {
   uint8_t payload[32];
} SGX_identity_t;

typedef struct SGX_attributes_t {
   uint8_t payload[16];
} SGX_attributes_t;

typedef struct SGX_secs {
    uint64_t size;
    uint64_t baseaddr;
    uint32_t ssaframesize;
    uint32_t misc_select;
    uint8_t  inited;

    RESERVE_BYTES(23);

    SGX_attributes_t attributes;
    SGX_identity_t mrenclave;

    RESERVE_BYTES(32);

    SGX_identity_t mrsigner;

    RESERVE_BYTES(96);

    uint16_t isvprodid;
    uint16_t isvsvn;
    RESERVE_BYTES(4); /* XXX explicit note of the padding that would be added
                         by gcc anyway to keep alignments (eid placement
                         is implementation specific so we don't care */

    uint64_t eid;

    __CTX
} SGX_secs;

#define SGX_ACTIVE 0x1
#define TCS_DBGOPTIN 0x1
typedef struct SGX_tcs {
    RESERVE_BYTES(8);

    uint64_t flags;
    uint64_t ossa;
    uint32_t cssa;
    uint32_t nssa;
    uint64_t oentry;
    uint64_t state; /* doc calls this reserved, but we'll use it for internal
                       state */
    uint64_t ofsbase;
    uint64_t ogsbase;
    uint32_t fslimit;
    uint32_t gslimit;

    RESERVE_BYTES(4024);
} SGX_tcs;
#ifndef __cplusplus
_Static_assert(sizeof(SGX_tcs) == PG_SIZE, "TCS should be one page!");
#endif

/* parameter structs used by the enclave requester */
typedef struct TCS_params {
    uint64_t flags;
    uint64_t ossa;
    uint32_t cssa;
    uint32_t nssa;
    uint64_t oentry;
    uint64_t ofsbase;
    uint64_t ogsbase;
    uint32_t fslimit;
    uint32_t gslimit;
    uint64_t tls_size;
} TCS_params;

typedef struct SECS_params {
    uint64_t size;
    uint64_t baseaddr;
    uint32_t ssaframesize;
    uint32_t miscSelect;
    SGX_attributes_t attributes;
    uint16_t isvprodid;
    uint16_t isvsvn;
} SECS_params;

/*******************************************************************************
 * EPCM, the EPCM maps phyisical addresses to EPCM entries
 * Each entry describes the attributes of a page in the EPC
 */

/* regular rwx bits */
#define SGX_READ  0x01
#define SGX_WRITE 0x02
#define SGX_EXEC  0x04

/* other page info bits */
#define SGX_PENDING  0x08
#define SGX_MODIFIED 0x10
#define SGX_BLOCKED  0x20
#define SGX_VALID 0x40

typedef enum {
    PT_SECS = 0,
    PT_TCS = 1,
    PT_REG = 2,
    PT_VA = 3,
    PT_TRIM = 4
} PageType;

#define SGX_RWX_MASK (0x7)


#define SEC_PT_OFFSET 8

#define SEC_PT_SECS (PT_SECS << SEC_PT_OFFSET)
#define SEC_PT_TCS  (PT_TCS  << SEC_PT_OFFSET)
#define SEC_PT_REG  (PT_REG  << SEC_PT_OFFSET)
#define SEC_PT_VA   (PT_VA   << SEC_PT_OFFSET)
#define SEC_PT_TRIM (PT_TRIM << SEC_PT_OFFSET)

#define SEC_PT_MASK (7 << SEC_PT_OFFSET)
#define SEC_GET_PT(val) ((PageType)((val & SEC_PT_MASK) >> SEC_PT_OFFSET))

typedef struct SecInfo {
   uint64_t flags;
   char     reserved[56];
} SecInfo;

static inline int SecInfo_check_reserved(SecInfo *secinfo)
{
   int i;
   for (i = 0; i < sizeof(secinfo->reserved); ++i) {
      if (secinfo->reserved[i])
         return 1;
   }
   return 0;
}


typedef struct SGX_key_t {
   uint8_t payload[16];
} SGX_key_t;

typedef struct SGX_userdata_t {
   uint8_t payload[64];
} SGX_userdata_t;

typedef struct cmac_t {
   uint8_t payload[16];
} cmac_t;

typedef struct SGX_PageInfo {
   uint64_t linaddr;
   uint64_t srcpge;
   SecInfo *secInfo;
   uint64_t secs;
} SGX_PageInfo;

typedef struct SGX_report {
   uint8_t  cpu_svn[16];
   uint32_t misc_select;

   RESERVE_BYTES(28);

   SGX_attributes_t attributes;
   SGX_identity_t mrenclave;

   RESERVE_BYTES(32);

   SGX_identity_t mrsigner;

   RESERVE_BYTES(96);

   uint16_t isv_prodid;
   uint16_t isv_svn;

   RESERVE_BYTES(60);

   SGX_userdata_t userdata;
   SGX_identity_t keyid;

   cmac_t mac;
} SGX_report;

typedef struct SGX_targetinfo {
   SGX_identity_t measurement;
   SGX_attributes_t attributes;

   RESERVE_BYTES(4);

   uint32_t miscs_select;

   RESERVE_BYTES(456);
} SGX_targetinfo;

typedef struct SGX_keyrequest {
   uint16_t keyname;
   uint16_t keypolicy;
   uint16_t isvsvn;

   RESERVE_BYTES(2);

   uint8_t  cpusvn[16];
   SGX_attributes_t  attributemask;
   SGX_identity_t  keyid;
   uint32_t miscmask;
} SGX_keyrequest;

typedef struct SGX_sigstruct {
   uint8_t  header[16];
   uint32_t vendor;
   uint32_t date;
   uint8_t  header2[16];
   uint32_t swdefined;

   RESERVE_BYTES(84);

   uint8_t  modulus[384];
   uint32_t exponent;
   uint8_t  signature[384];
   uint32_t miscselect;
   uint32_t miscmask;

   RESERVE_BYTES(20);

   uint8_t         attributes[16];
   uint8_t         attributemask[16];
   SGX_identity_t  enclavehash;

   RESERVE_BYTES(32);

   uint16_t isvprodid;
   uint16_t isvsvn;

   RESERVE_BYTES(12);

   uint8_t q1[384];
   uint8_t q2[384];
} SGX_sigstruct;

typedef struct SGX_einittoken {
   uint32_t valid;

   RESERVE_BYTES(44);

   uint8_t attributes[16];
   uint8_t mrenclave[32];

   RESERVE_BYTES(32);

   uint8_t mrsigner[32];

   RESERVE_BYTES(32);

   uint8_t  cpusvnle[16];
   uint16_t isvprodidle;
   uint16_t isvsvnle;

   RESERVE_BYTES(24);

   uint32_t maskedmiscselectle;
   uint8_t maskedattributesle[16];
   uint8_t keyid[32];
   cmac_t mac;
} SGX_einittoken;


/* linux syscall params */
typedef struct SGX_setup {
   const char *filename;
   char **argv;
   char **envp;
   SECS_params *secs;
   TCS_params *tcs;
   SGX_sigstruct *sigstruct;
} SGX_setup;

typedef struct SGX_init_state {
    unsigned long base;
    unsigned long size;
    unsigned long mmap_line;
    unsigned long sp;
    unsigned long brk;
    unsigned long tls_base;
    SGX_tcs* tcs;
    uint8_t  mrenclave[32];
} SGX_init_state;

/*
_Static_assert(sizeof(SGX_ssa_frame) % PG_SIZE == 0, "SSA Frame should be "
                                                     "a multiple of 4k");
*/
#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
