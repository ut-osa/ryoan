#ifndef _ENCLAVE_OPS_H
#define _ENCLAVE_OPS_H

#include "sgx.h"

long syscall_eexit(long, long, long, long, long, long);
void bounce_eexit(void);
void eexit_final(long);
void ereport(SGX_targetinfo *targetinfo, SGX_userdata_t *userdata,
             SGX_report *report);
int egetkey(SGX_keyrequest *request, SGX_key_t *key);

#endif /*_ENCLAVE_OPS_H*/
