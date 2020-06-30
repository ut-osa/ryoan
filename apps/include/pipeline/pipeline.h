#ifndef _PIPELINE_H
#define _PIPELINE_H
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include "pipeline/channel.h"

#ifdef __cplusplus
extern "C" {
#endif

struct pipe_ctx;
#define FAKE_KEY_HEX \
  "b52c505a37d78eda5dd34f20c22540ea1b58963cf8e5bf8ffa85f9f2492505b4"

typedef struct {
  unsigned id;
  int visited; /* useful in graph construction */
  int nin;
  int *in_ids;
  int nout;
  char **out_ids;
  int argc;
  char **argv;
} FittingSpec;

typedef struct {
  unsigned n;
  ChannelDesc **channels;
} WorkSpec;

FittingSpec *FittingSpec_parse(const char *);
void FittingSpec_free(FittingSpec *);

WorkSpec *WorkSpec_parse(const char *);
void WorkSpec_free(WorkSpec *);

WorkSpec *fitting_to_workspec(FittingSpec *fit, FittingSpec *id_map[],
                              int form_pred, int to_pred, int from_succ,
                              int to_succ);

char *WorkSpec_serialize(WorkSpec *);

int feed_data_plain(int top_in, int top_out, int bottom_in, int bottom_out,
                    const void *data, size_t len);
int feed_data(int top_in, int top_out, const void *data, size_t len,
              Channel_Type type);
int feed_data_desc(int top_in, int top_out, const void *desc, size_t dlen,
                   const void *data, size_t len, Channel_Type type,
                   bool send_to_nacl);
void sleep_until_ready(struct pipe_ctx *ctx);

int feed_data_desc_chan_no_spec(channel_t **top, int n_chans, const void *desc,
                                size_t dlen, const void *data, size_t len,
                                bool enc);

int feed_data_desc_chan(channel_t **top, const WorkSpec *spec, const void *desc,
                        size_t dlen, const void *data, size_t len, bool enc);

int feed_data_desc_chan_no_model(channel_t *chan, const void *desc, size_t dlen,
                                 const void *data, size_t len);

uint8_t *get_nonce(void);

uint8_t *get_fake_key(void);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif
