#ifndef _PIPELINE_H
#define _PIPELINE_H
#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>
#include "pipeline/channel.h"

#ifdef __cplusplus
extern "C"
{
#endif
struct pipe_ctx;

typedef struct {
    unsigned  id;
    int  visited; /* useful in graph construction */
    int  nin;
    int *in_ids;
    int  nout;
    int *out_ids;
    int  argc;
    char **argv;
} FittingSpec;

typedef struct {
    unsigned n;
    ChannelDesc **channels;
} WorkSpec;

FittingSpec *FittingSpec_parse(const char*);
void FittingSpec_free(FittingSpec *);

WorkSpec *WorkSpec_parse(const char*);
void WorkSpec_free(WorkSpec *);

WorkSpec *fitting_to_workspec(FittingSpec *fit, FittingSpec *id_map[],
        int form_pred, int to_pred, int from_succ, int to_succ);

char *WorkSpec_serialize(WorkSpec *);

int feed_data_plain(int top_in, int top_out, int bottom_in, int bottom_out,
        const void *data, uint64_t len);
int feed_data(int top_in, int top_out, const void *data, uint64_t len,
        Channel_Type type);
int feed_data_desc(int top_in, int top_out, const void *desc, uint64_t dlen,
        const void *data, uint64_t len, Channel_Type type);

int wait_for_chan(bool output_app_label, struct pipe_ctx *ctx, uint8_t** desc,
    uint64_t *dlen, uint8_t** data, uint64_t *len);
#ifdef __cplusplus
} /* extern "C" */
#endif
#endif
