#ifndef NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_SGX_PERF_H_
#define NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_SGX_PERF_H_ 1

#include<stddef.h>
#include<stdio.h>

void sgx_perf_enable(void);
int sgx_perf_disable(void);
int sgx_perf_snprint(char *buf, size_t size);
int sgx_perf_fprint(FILE *);
int sgx_perf_print(void);

#endif
