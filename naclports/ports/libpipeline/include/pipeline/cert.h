/*
 * pipeline_cert.h
 *
 * Operations used by pipeline working programs
 * Handles log ops transparently
 */
#ifndef _PIPELINE_CERT_H
#define _PIPELINE_CERT_H
#include <unistd.h>
#include "sgx.h"
#include "pipeline/pipeline.h"

#ifdef __cplusplus
extern "C"
{
#endif
typedef struct pipe_ctx *cert_ctx;
cert_ctx alloc_ctx(void);

/*
 * Shortcut to calulate the mrenclave of this enclave and send it over fd
 */
int send_mrenclave(int in_fd, int out_fd);

/*
 * One time setup
 *
 * @returns 0 on success or -1 in case of error
 */
int setup_for_cert(WorkSpec *spec);

/*
 * Get results
 * @param ctx: updated to point to context for data
 * @param len:  size of data in bytes after return
 *
 * @returns pointer to data read, NULL on error
 *
 * XXX causes assertion failure if len < data size (determined by sender)
 */
unsigned char *get_result(cert_ctx ctx, int64_t *len);
int get_result_desc(cert_ctx ctx, char **desc, int64_t *dlen,
        unsigned char **data, int64_t *len);

/*
 * Check log, verify macs, ensure correctness, etc.
 * @param ctx: context to verify
 *
 * @returns 0 -> log good; 1 -> log bad
 */
int check_log(cert_ctx ctx);

/*
 * Print the content of a log to stderr
 * @param ctx: context with log to print
 *
 */
void print_log(cert_ctx ctx);

/*
 * List ops; extracts the identity of every enclave that operated on the log
 * @param ctx: context (holds log)
 * @param op_list: buffer to hold identities
 * @param nops: number of identities allocated in op_list
 *
 * @returns: >=0, number of identities stored in op_list, -1 on error
 */
int list_ops(cert_ctx ctx, SGX_identity_t *op_list, uint64_t nops);

void free_ctx(struct pipe_ctx* ctx);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif
