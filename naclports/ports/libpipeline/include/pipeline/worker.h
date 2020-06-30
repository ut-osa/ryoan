/*
 * pipeline_worker.h
 *
 * Operations used by pipeline working programs
 * Handles log ops transparently
 */
#ifndef _PIPELINE_WORKER_H
#define _PIPELINE_WORKER_H

#include <unistd.h>
#include "pipeline/pipeline.h"

#ifdef __cplusplus
extern "C" {
#endif



typedef struct pipe_ctx *work_ctx;
work_ctx alloc_ctx();

/*
 * One time setup
 *
 * Synchronous: Will not return 0 unless all channels were successfully
 * established.
 *
 * @returns 0 on success or -1 in case of error
 */
int setup_for_work(WorkSpec *spec);

/*
 * Retreive Work from pipeline
 * @param ctx:  updated to point to context for data
 * @param data: buffer to fill with input
 * @param len:  holds size of result after return
 *
 * @returns the number of bytes read (>0) on success or -1 in case of error
 *
 * XXX causes assertion failure if len < data size (determined by sender)
 */
unsigned char *get_work(work_ctx ctx, int64_t *len);
int get_work_desc(work_ctx ctx, char **desc, int64_t *dlen,
        unsigned char **data, int64_t *len);

#ifndef __native_client__
int get_work_desc_buffer(unsigned char *buffer,
    size_t size,
    size_t start,
    size_t *end,
    char **desc,
    ssize_t *dlen,
    unsigned char** data,
    ssize_t* len);

int get_work_desc_single(work_ctx ctx, char **desc, int64_t *dlen,
        unsigned char **data, int64_t* len);
#endif
/*
 * Send completed work down the pipeline
 * @param ctx:  pointer to the context for work (given by get_work)
 * @param data: buffer containing output
 * @param len:  number of bytes to send from data
 *
 * @sideeffect: ctx is destroyed
 *
 * @returns 0 on success or -1 in case of error
 */
int put_work(work_ctx ctx, const unsigned char *data, uint64_t len);
int put_work_desc(work_ctx ctx, const char *desc, uint64_t dlen,
        const unsigned char *data, uint64_t len);

/*
 * Tear down and flush everything
 *
 * @returns 0 on success or -1 in case of error
 */
int finish_work(int);
void free_ctx(work_ctx ctx);

#ifdef __cplusplus
}
#endif

#endif
