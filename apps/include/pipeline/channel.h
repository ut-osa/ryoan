#ifndef _CHANNEL_H_
#define _CHANNEL_H_
#include <assert.h>
#include <err.h>
#include <stdio.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum Channel_Type {
  PLAIN_CHANNEL,
  DH_CHANNEL,
  UNDEF_CHANNEL = -1,
} Channel_Type;
const char *chan_type_str(Channel_Type type);
Channel_Type str_chan_type(const char *str);

typedef enum Flow_Dir {
  IN,
  OUT,
  UNDEF = -1,
} Flow_Dir;
const char *flow_dir_str(Flow_Dir type);
Flow_Dir str_flow_dir(const char *str);

typedef struct ChannelDesc {
  char *other_path;
  // output size can be a function of input. e.g., "a1b2" means (1 * input + 2)
  char *size_func;
  int in_fd;
  int out_fd;
  Flow_Dir dir;
  Channel_Type type;
} ChannelDesc;

typedef struct channel_t channel_t;
typedef ssize_t(_chan_read_op)(channel_t *chan, void *buff, size_t size);
typedef ssize_t(_chan_write_op)(channel_t *, const void *, size_t);
typedef void(_chan_free_op)(channel_t *, int close);
typedef int(_chan_ready_op)(channel_t *, Flow_Dir);
typedef int *(_chan_get_in_fd)(channel_t *);
typedef _chan_read_op *chan_read_op;
typedef _chan_write_op *chan_write_op;
typedef _chan_free_op *chan_free_op;
typedef _chan_ready_op *chan_ready_op;
typedef _chan_get_in_fd *chan_get_in_fd;

struct channel_t {
  const chan_read_op read;
  const chan_write_op write;
  const chan_free_op free;
  const chan_ready_op ready;
  const chan_get_in_fd get_in_fd;
  void *opaque;
};

/* functions for interfacing with channels */
static inline ssize_t chan_read(channel_t *chan, void *buff, size_t size) {
  assert(chan);
  return chan->read(chan, buff, size);
}

static inline ssize_t chan_write(channel_t *chan, const void *buff,
                                 size_t size) {
  return chan->write(chan, buff, size);
}

static inline void free_channel(channel_t *chan, int close) {
  chan->free(chan, close);
}

static inline int chan_ready(channel_t *chan, Flow_Dir dir) {
  return chan->ready(chan, dir);
}
static inline int *get_in_fd(channel_t *chan) {
  if (chan->get_in_fd) return chan->get_in_fd(chan);
  return 0;
}

channel_t *build_channel_type(int in, int out, Channel_Type type);
channel_t *build_channel_desc(ChannelDesc *desc);
channel_t *build_plain_text_channel(int in, int out);
channel_t *build_d_h_channel(int in, int out);
int write_nbytes(channel_t *chan, const void *bytes, size_t len);
int read_nbytes(channel_t *chan, void *bytes, size_t len);

#ifdef __cplusplus
}
#endif
#endif
