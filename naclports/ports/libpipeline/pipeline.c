#include <string.h>
#include <assert.h>

#ifndef NOT_SGX
#include "sgx.h"
#include "enclaveops.h"
#endif

#ifdef __native_client__
#include <request.h>
#else
#include <poll.h>
#include <sys/epoll.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#define MAXEVENTS 64
#endif

#include "pipeline/pipeline.h"
#include "pipeline/worker.h"
#include "pipeline/cert.h"
#include "pipeline/errors.h"
#include "pipeline/channel.h"
#include "pipeline/utils.h"


/* Macros so we can be agnostic to channel type */
#define SETUP_CHANNEL(...)    build_d_h_channel(__VA_ARGS__)
#define TEARDOWN_CHANNEL(...) free_channel(__VA_ARGS__)
#define MAX_CHANNELS 64


typedef struct log_entry_t {
   hash_t hash;
   SGX_report report;
} log_entry_t;

typedef struct log_t {
   log_entry_t *first;
   uint64_t len;
   uint64_t cap;
} log_t;

struct pipe_ctx {
    log_t log;
    SGX_identity_t certifier;
    unsigned nin;
    unsigned nout;
    channel_t *in[MAX_CHANNELS];
    channel_t *out[MAX_CHANNELS];
};

/* channels to predicessor and successor */
static struct {
    unsigned nin;
    unsigned nout;
    channel_t *in[MAX_CHANNELS];
    channel_t *out[MAX_CHANNELS];
} pipeline_state;

typedef struct d_h_key_t{
    unsigned char payload[20];
} d_h_key_t;

static
int read_nbytes(channel_t *chan, void *bytes, uint64_t len) {
   int64_t ret;
   uint64_t _read = 0;
   uint64_t to_read = len, next_chunk;

   if (!len) {
      return 0;
   }
   /* Have to break this into chunks in order to avoid overflowing the
      syscall buffer */
   do {
      next_chunk = to_read < 4096 ? to_read : 4096;
      ret = chan_read(chan, bytes + _read, next_chunk);
      if (ret < 0) {
         if (errno == EAGAIN || errno == EWOULDBLOCK)
             continue;
         perror("read_nbytes");
         return -1;
      }
      _read += ret;
      to_read -= ret;
   } while(_read < len && ret);

   if (_read != len) {
     return -1;
   }

   return 0;
}


static
int write_nbytes(channel_t *chan, const void *bytes, uint64_t len)
{
   int64_t ret;
   uint64_t written = 0;

   if (!len) {
      return 0;
   }

   do {
      ret = chan_write(chan, bytes + written, len - written);
      if (ret < 0) {
         if (errno == EAGAIN || errno == EWOULDBLOCK)
             continue;
         return -1;
      }
      written += ret;
   } while(written < len);

   if (written != len) {
      return -1;
   }

   return 0;
}

static inline int32_t ReadNBytes(int fd, uint8_t* buf, uint64_t len, int try) {
  uint64_t bytes = 0;
  ssize_t count = 0;

  while (bytes < len) {
    count = read(fd, buf + bytes, len - bytes);
    if (count > 0) {
      bytes += (uint64_t) count;
    } else {
      if (errno == EINTR) {
        continue;
      } if (!try && errno == EAGAIN) {
        continue;
      } else {
        return -1;
      }
    }
  }
  return 0;
}

static inline
int send_string(channel_t *chan, const void *string, uint64_t len)
{
   int ret = write_nbytes(chan, &len, sizeof(uint64_t));
   if (ret) return ret;
   return write_nbytes(chan, string, len);
}

struct pipe_ctx *alloc_ctx()
{
    struct pipe_ctx *ctx;

    ctx = malloc(sizeof(struct pipe_ctx));
    if (!ctx) {
        set_error("malloc ctx");
        return NULL;
    }

    memset(ctx, 0, sizeof(struct pipe_ctx));

    ctx->nin = pipeline_state.nin;
    ctx->nout = pipeline_state.nout;
    memcpy(ctx->in,  pipeline_state.in,  sizeof(channel_t) * ctx->nin);
    memcpy(ctx->out, pipeline_state.out, sizeof(channel_t) * ctx->nout);

    return ctx;
}

static inline
int read_certifier_id(channel_t *chan, SGX_identity_t *id)
{
    return read_nbytes(chan, id, sizeof(*id));
}

/* TODO: Make this Async */
static
unsigned char *recv_string_async(channel_t *chan, int64_t *len)
{
    unsigned char *buffer;
   if (read_nbytes(chan, len, sizeof(int64_t))) {
       return NULL;
   }

   buffer = malloc(*len);
   if (!buffer) {
       set_error("malloc buffer");
       return NULL;
   }

   if (read_nbytes(chan, buffer, *len) == 0) {
       if (buffer == NULL) {
            // eprintf("Does not get buffer\n");
       }
       return buffer;
   }
   return NULL;
}

unsigned char *recv_string(channel_t *chan, int64_t *len)
{
    unsigned char *buffer;
   if (read_nbytes(chan, len, sizeof(int64_t))) {
       return NULL;
   }

   buffer = malloc(*len);
   if (!buffer) {
       set_error("malloc buffer");
       return NULL;
   }

   if (read_nbytes(chan, buffer, *len) == 0) {
       return buffer;
   }
   return NULL;
}

int recv_alloc_log_single(channel_t *in, log_t *log)
{
   uint64_t size;
   int ret = read_nbytes(in, &size, sizeof(uint64_t));
   if (ret) return ret;
   log->len = size / sizeof(log_entry_t);


   if (!size) {
      /* we must be the first, let's get the ball rolling! */
      log->cap = 2; /* 2 entries one for inital data hast and the other for
                       "normal" operation */
      log->first = malloc(log->cap * sizeof(log_entry_t));
      if (!log->first) {
          return -1;
      }
   }
   else {
      log->cap = log->len + 1;
      log->first = malloc(log->cap * sizeof(log_entry_t));
      if (!log->first) {
          return -1;
      }
      ret = read_nbytes(in, log->first, size);
      if (ret) {
         free(log->first);
      }
   }
   return ret;
}

static
int compute_log_hash(hash_t *hash, const log_t *log, uint64_t n)
{
   uint64_t total_size;
   assert(log->cap >= n);

   total_size = sizeof(log_entry_t) * n;

   /* assume the data hash of n+1 should be involved as well */
   total_size += sizeof(hash_t);
   return compute_hash(hash, log->first, total_size);
}


static
int gen_report(struct pipe_ctx *ctx, SGX_report *report)
{
   SGX_userdata_t userdata __align(128);
   SGX_targetinfo targetinfo __align(128) = {
       .measurement = ctx->certifier,
   };
   memset(&userdata, 0, sizeof(userdata));
   if (compute_log_hash((hash_t *)&userdata, &ctx->log, ctx->log.len)) {
      set_error("compute log hash");
      return -1;
   }
   return _ereport(report, &userdata, &targetinfo);
}

static
int log_update(struct pipe_ctx *ctx, const void *data, uint64_t len)
{
    assert(ctx->log.len < ctx->log.cap);
   log_entry_t *next =  ctx->log.first + ctx->log.len;

   if (compute_hash(&next->hash, data, len)) {
      set_error("hash");
      return -1;
   }

   if (gen_report(ctx, &next->report)) {
      set_error("report");
      return -1;
   }

   ctx->log.len++;

   return 0;
}

static inline
int start_pipeline(struct pipe_ctx *ctx, const void *data, uint64_t len)
{
   assert(ctx->log.len == 0);
   return log_update(ctx, data, len);
}

void free_ctx(struct pipe_ctx* ctx)
{
    /* log entries are always allocated as one big slab */
    if (ctx->log.first) {
        free(ctx->log.first);
    }
    free(ctx);
}

static inline
int cmp_hash(hash_t *lhs, hash_t *rhs)
{
   return memcmp(lhs, rhs, sizeof(hash_t));
}

int check_in_hash(log_t *log, unsigned char *data, uint64_t len) {
    hash_t hash;
    if (compute_hash(&hash, data, len)) {
        return -1;
    }
    if (cmp_hash(&hash, &log->first[log->len - 1].hash)) {
        return -1;
    }
    return 0;
}

static inline
int send_log_single(channel_t *out, log_t *log)
{
   return send_string(out, log->first, log->len * sizeof(log_entry_t));
}

static
void dump_report(const SGX_report* report) {
   int i;
   printf("userdata: ");
   /* XXX SGX_userdata_t is an array of uint8_t */
   for (i = 0; i < sizeof(SGX_userdata_t); ++i) {
      printf("%hhx", report->userdata.payload[i]);
   }
   printf("\n");
}

static
void dump_hash_t(const hash_t* hash) {
   int i;
   printf("hash: ");
   for (i = 0; i < sizeof(hash_t); ++i) {
      printf("%hhx", hash->payload[i]);
   }
   printf("\n");
}

static
void dump_log_entry_t(const log_entry_t* entry) {
   printf("\t");
   dump_hash_t(&entry->hash);
   printf("\t");
   dump_report(&entry->report);
}

static
void dump_log_t(const log_t *log) {
   int i;
   printf("entries: %lu\nlog:\n", log->len - 1);
   for (i = 0; i < log->len; ++i) {
      dump_log_entry_t(log->first + i);
   }
}

static
int log_check_macs(log_t *log)
{
   log_entry_t *entry = log->first;
   log_entry_t *end = log->first + log->len;
   for (; entry < end; ++entry) {
      if (verify_mac(&entry->report)) {
         set_error("bad mac");
         return -1;
      }
   }
   return 0;
}

static
int log_check_hashes(log_t *log)
{
   int i = log->len - 1;
   log_entry_t *entry = log->first + i;
   hash_t hash;
   hash_t *stored_hash;

   for(; entry >= log->first; --entry, --i) {
      assert(i >= 0);
      stored_hash = (hash_t*)&entry->report.userdata;
      if (compute_log_hash(&hash, log, i)) {
         set_error("compute hash");
         return -1;
      }
      if (cmp_hash(stored_hash, &hash)) {
         set_error("hash mismatch");
         return -1;
      }
   }
   return 0;
}

static
int data_check_hashes(log_t *log)
{
   log_entry_t *entry = log->first + log->len - 1;
   hash_t *stored_hash;

   assert(log->len);
   stored_hash = &entry->hash;

   while (--entry >= log->first) {
      if (cmp_hash(stored_hash, &entry->hash)) {
         return -1;
      }
   }
   return 0;
}


/*
 ****** pipeline init functions ********
 */
int feed_data(int top_in, int top_out, const void *data, uint64_t len,
        Channel_Type type)
{
   int ret = 0;
   channel_t *top;

   switch (type) {
        case DH_CHANNEL:
            top = build_d_h_channel(top_in, top_out);
            break;
        case PLAIN_CHANNEL:
            top = build_plain_text_channel(top_in, top_out);
            break;
        default:
           eprintf("unknown channel type\n");
           return -1;
   }

   if (!top) {
       return -1;
   }

   ret = send_string(top, data, len);
   if (ret) {
      error("send data");
      goto done;
   }

 done:
   free_channel(top, 0);
   return ret;
}

int feed_data_desc(int top_in, int top_out, const void *desc, uint64_t dlen,
        const void *data, uint64_t len, Channel_Type type)
{
   int ret = 0;
   channel_t *top;

   switch (type) {
        case DH_CHANNEL:
            top = build_d_h_channel(top_in, top_out);
            break;
        case PLAIN_CHANNEL:
            top = build_plain_text_channel(top_in, top_out);
            break;
        default:
           eprintf("unknown channel type\n");
           return -1;
   }

   if (!top) {
       return -1;
   }
   ret = send_string(top, desc, dlen);
   if (ret) {
      error("send data");
      goto done;
   }

   ret = send_string(top, data, len);
   if (ret) {
      error("send data");
      goto done;
   }

 done:
   free_channel(top, 0);
   return ret;
}

/* like feed data but always uses a plaintext channel to start */
int feed_data_plain(int top_in, int top_out, int bottom_in, int bottom_out,
        const void *data, uint64_t len)
{
   int ret = 0;
   SGX_identity_t mrenclave; /* measurement of the certifier */
   uint64_t log_len = 0; /* because we're at the head */
   channel_t *top, *bottom;

   top = build_plain_text_channel(top_in, top_out);
   if (!top) {
       return -1;
   }
   bottom = build_plain_text_channel(bottom_in, bottom_out);
   if (!bottom) {
       free_channel(top, 0);
       return -1;
   }

   /* XXX wait to get the measurement
      HACK since we may be updating code a lot and don't have the ability to
      recompute quicly */
   ret = read_nbytes(bottom, &mrenclave, sizeof(SGX_identity_t));
   if (ret) {
      set_error("read mrenclave");
      goto done;
   }
   ret = write_nbytes(top, &mrenclave, sizeof(SGX_identity_t));
   if (ret) {
      set_error("read mrenclave");
      goto done;
   }

   ret = send_string(top, data, len);
   if (ret) {
      set_error("send data");
      goto done;
   }

   ret = chan_write(top, &log_len, sizeof(uint64_t));
   if (ret) {
      set_error("send log");
      goto done;
   }

 done:
   free_channel(top, 0);
   free_channel(bottom, 0);
   return ret;
}


/*
 ****** normal pipeline worker functions ********
 */
int setup_for_work(WorkSpec *spec)
{
    int i;
    channel_t *chan;
    Flow_Dir dir;

    for (i = 0; i < spec->n; ++i) {
        dir = spec->channels[i]->dir;
        if (!(chan = build_channel_desc(spec->channels[i]))) {
            goto err;
        }
        if (dir == IN) {
            assert(pipeline_state.nin < MAX_CHANNELS);
            pipeline_state.in[pipeline_state.nin++] = chan;
        }
        if (dir == OUT) {
            assert(pipeline_state.nout < MAX_CHANNELS);
            pipeline_state.out[pipeline_state.nout++] = chan;
        }
    }
    return 0;
 err:
    finish_work(0);
    return -1;
}

int finish_work(int close) {
    int i;
    for (i = 0; i < pipeline_state.nin; ++i) {
        TEARDOWN_CHANNEL(pipeline_state.in[i], close);
    }
    for (i = 0; i < pipeline_state.nout; ++i) {
        TEARDOWN_CHANNEL(pipeline_state.out[i], close);
    }
    return 0;
}

unsigned char *get_work(work_ctx ctx, int64_t* len)
{
    unsigned char *data = NULL;
    int i;

    /*
    if (read_certifier_id(_ctx->in, &_ctx->certifier)) {
        set_error("read certifier");
        goto err_free_ctx;
    }
    */


    for (i = 0; i < ctx->nin; ++i) {
        data = recv_string_async(ctx->in[i], len);
        if (data) {
            break;
        }
    }
    if (!data) {
        return NULL;
    }

    /*
    if (recv_alloc_log(_ctx)) {
        set_error("recv alloc log");
        goto err_free_data;
    }

    if (_ctx->log.len == 0) {
        if (start_pipeline(_ctx, data, *len)) {
            set_error("start pipeline");
            goto err_free_data;
        }
    } else if (check_in_hash(&_ctx->log, data, *len)) {
        set_error("Input and hash don't match");
        goto err_free_data;
    }
    */


    /* all is well; publish */
    return data;
 //err_free_data:
    free(data);
    return NULL;
}

#ifndef __native_client__
int makeFdNonBlocking(int fd) {
  int flags, s;
  flags = fcntl(fd, F_GETFL, 0);
  if (flags == -1) {
    fprintf(stderr, "fcntl: %s\n", strerror(errno));
    return -1;

  }
  flags |= O_NONBLOCK;
  s = fcntl(fd, F_SETFL, flags);
  if (s == -1) {
    fprintf(stderr, "fcntl: %s\n", strerror(errno));
    return -1;
  }
  return 0;
}

int get_work_desc_buffer(unsigned char *buffer,
    size_t size,
    size_t start,
    size_t *end,
    char **desc,
    ssize_t *dlen,
    unsigned char** data,
    ssize_t* len) {
  char *_desc = 0;
  unsigned char *_data = 0;

  if (size - start > sizeof(ssize_t)) {
    memcpy(dlen, buffer + start, sizeof(ssize_t));
    printf("desc size is %d\n", (int)*dlen);
    if (*dlen < size - start - sizeof(ssize_t)) {
      _desc = (char*) malloc(*dlen);
      memcpy(_desc, buffer + start + sizeof(ssize_t), *dlen);
      printf("desc is %s", _desc);
      if (size - start - sizeof(ssize_t) - *dlen > sizeof(ssize_t)) {
        memcpy(len, buffer + start + sizeof *dlen + *dlen, sizeof(ssize_t));
        printf("Get data len: %d\n", (int)*len);
        printf("rest length: %d\n", (int)(size - sizeof(ssize_t)*2 - *dlen));
        if (size - start - sizeof(ssize_t)*2 - *dlen >= *len) {
          _data = (unsigned char*) malloc(*len);
          memcpy(_data, buffer + start + sizeof(ssize_t)*2 + *dlen, *len);
          printf("----get all expected data-------\n");
          *data = _data;
          *desc = _desc;
          *end = start + sizeof(ssize_t)*2 + *dlen + *len;
          return 0;
        } else {
          return -1;
        }
      } else {
        return -1;
      }
    } else {
      return -1;
    }
  } else {
    printf("size has to be at least: %d\n", (int)sizeof(ssize_t));
    memcpy(dlen, buffer, sizeof(ssize_t));
    printf("expected dlen is %d\n", (int)*dlen);
    return -1;
  }
}
#endif

int wait_for_chan(bool output_app_label, struct pipe_ctx *ctx, uint8_t** desc,
    uint64_t *dlen, uint8_t** data, uint64_t *len) {
#ifdef __native_client__
  (void)ctx;
  (void)desc;
  (void)dlen;
  (void)data;
  (void)len;
  return wait_for_request(output_app_label);
#else
  struct epoll_event event;
  struct epoll_event *events;
  int32_t efd, s;
  int i, n, retval;

  efd = epoll_create1(0);
  if (efd == -1) {
    fprintf(stderr, "epoll_create: %s\n", strerror(errno));
    return -1;
  }
  for (i = 0; i < ctx->nin; i++) {
    int in_fd = ((int *)(ctx->in[i]->opaque))[0];
    eprintf("Monitoring %d\n", in_fd);
    s = makeFdNonBlocking(in_fd);
    if (s == -1) {
      fprintf(stderr, "Fail to make fd non-blocking\n");
      retval = -1;
      goto done;
    }
    event.data.fd = in_fd;
    event.events = EPOLLIN | EPOLLET;
    s = epoll_ctl(efd, EPOLL_CTL_ADD, in_fd, &event);
    if (s == -1) {
      fprintf(stderr, "epoll_ctl: %s\n", strerror(errno));
      retval = -1;
      goto done;
    }
  }
  events = calloc(MAXEVENTS, sizeof event);
  while (1) {
    eprintf("Before epoll wait\n");
    n = epoll_wait(efd, events, MAXEVENTS, -1);
    eprintf("Get %d events\n", n);
    for (i = 0; i < n; i++) {
      if (((events[i].events & EPOLLERR) ||
            (events[i].events & EPOLLHUP)) &&
          (!(events[i].events & EPOLLIN))) {
        eprintf("epoll: %s\n", strerror(errno));
      } else {
        uint64_t desc_size;
        uint8_t* _desc;
        uint64_t data_size;
        uint8_t* _data;
        retval = ReadNBytes(events[i].data.fd,
            (uint8_t *)&desc_size,
            sizeof(uint64_t), 0);
        if (retval == 0) {
          _desc = (uint8_t *) malloc(desc_size);
          if (_desc == NULL) {
            fprintf(stderr, "fail to create\n");
            retval = -1;
            goto done;
          }
          retval = ReadNBytes(events[i].data.fd,
              _desc, desc_size, 0);
          if (retval != 0) {
            retval = -1;
            goto done;
          }
          retval = ReadNBytes(events[i].data.fd,
              (uint8_t *)&data_size,
              sizeof(uint64_t), 0);
          _data = (uint8_t *) malloc(data_size);
          if (_data == NULL) {
            fprintf(stderr, "fail to create\n");
            retval = -1;
            goto done;
          }
          retval = ReadNBytes(events[i].data.fd,
              _data, data_size, 0);
          if (retval != 0) {
            retval = -1;
            goto done;
          }
          *desc = _desc;
          *dlen = desc_size;
          *data = _data;
          *len = data_size;
          retval = 0;
          goto done;
        } else {
          retval = -1;
          goto done;
        }
      }
    }
  }
 done:
  free(events);
  close(efd);
  return retval;
#endif
}

#ifndef __native_client__
static
channel_t *sleep_until_ready_common(channel_t **chans, int n)
{
    int i;
    int negated = 0;
    struct pollfd fds[n];
    int *fdp[n];
    channel_t *chan = NULL;
    memset(fds, 0, sizeof(struct pollfd) * n);

    for (i = 0; i < n; ++i) {
        fdp[i] = ((int *)(chans[i]->opaque))[0];
        fds[i].fd = *fdp[i];
        fds[i].events |= POLLIN;
    }
    while (1) {
        if (poll(fds, n, -1) > 0) {
            for (i = 0; i < n; ++i) {
                if (fds[i].revents) {
                    if (fds[i].revents & POLLHUP) {
                        fds[i].fd = -1;
                        negated++;
                    }
                    if (fds[i].revents & POLLIN) {
                        chan = chans[i];
                        break;
                    }
                    fds[i].revents = 0;
                }
            }
            if (chan || (negated >= n)) {
                break;
            }
        }
    }
    return chan;
}

void sleep_until_ready(work_ctx ctx)
{
    sleep_until_ready_common(ctx->in, ctx->nin);
}
#endif

/* Return 1 if no work but some channels un available
 *       -1 if no work and all channels closed
 *       0 otherwise (success)
 */
int get_work_desc(work_ctx ctx, char **desc, int64_t *dlen,
        unsigned char **data, int64_t* len)
{
    char *_desc = 0;
    unsigned char *_data;
    int i, ret;
    int not_ready = 0;
    static used_in[10];

    assert(ctx->nin < 10);

    for (i = 0; i < ctx->nin; ++i) {
#ifdef __native_client__
      _desc = (char *)recv_string_async(ctx->in[i], dlen);
      if (_desc) break;
#else
        if (used_in[i]) continue;
        if (!chan_ready(ctx->in[i], IN)) {
            not_ready = 1;
        } else {
          used_in[i] = 1;
          _desc = (char *)recv_string_async(ctx->in[i], dlen);
          assert(_desc);
          break;
       }
#endif
    }

    if (!_desc) {
        if (not_ready) return 1;
        else {
#ifndef __native_client__
            /* Read from every descriptor */
            for (i = ctx->nin - 1; i >= 0; i--) {
               used_in[i] = 0;
            }
#endif
            return -1;
        }
    }

    _data = recv_string_async(ctx->in[i], len);
    if (!_data) {
        goto err_free_desc;
    }

    /* all is well; publish */
    *data = _data;
    *desc = _desc;
    return 0;
 err_free_desc:
    free(_desc);
    assert(0 && "major problem");
    return -1;
}

#ifndef __native_client__
int get_work_desc_single(work_ctx ctx, char **desc, int64_t *dlen,
        unsigned char **data, int64_t* len)
{
    char *_desc = 0;
    unsigned char *_data;
    int i, ret;
    int not_ready = 0;
    static used_in[10];

    assert(ctx->nin < 10);
    // eprintf("number of in channel is: %d\n", ctx->nin);

    _desc = (char *)recv_string_async(ctx->in[0], dlen);
    eprintf("get desclen: %ld\n", *dlen);

    if (!_desc) {
        return -1;
    }

    _data = recv_string_async(ctx->in[0], len);
    eprintf("get datalen: %ld\n", *len);
    if (!_data) {
        goto err_free_desc;
    }

    /* all is well; publish */
    *data = _data;
    *desc = _desc;
    return 0;
 err_free_desc:
    free(_desc);
    assert(0 && "major problem");
    return -1;
}
#endif

int put_work(work_ctx ctx, const unsigned char *data, uint64_t len)
{
    int i;
    /*
   if (log_update(ctx, data, len)) {
       set_error("log update");
       return -1;
   }

    if (write_nbytes(ctx->out, &ctx->certifier, sizeof(ctx->certifier))) {
        set_error("send mrenclave");
        return -1;
    }
   */

   for (i = 0; i < ctx->nout; ++i) {
       if (send_string(ctx->out[i], data, len)) {
          set_error("send data");
          return -1;
       }
   }

   /*
   if (send_log(ctx)) {
      set_error("send log");
      return -1;
   }
   */

   return 0;
}

/* desc is ussually a JSON string */
int put_work_desc(work_ctx ctx, const char *desc, uint64_t dlen,
        const unsigned char *data, uint64_t len)
{
    int i;
    /*
   if (log_update(ctx, data, len)) {
       set_error("log update");
       return -1;
   }

    if (write_nbytes(ctx->out, &ctx->certifier, sizeof(ctx->certifier))) {
        set_error("send mrenclave");
        return -1;
    }
   */
   for (i = 0; i < ctx->nout; ++i) {
       if (send_string(ctx->out[i], desc, dlen)) {
          eprintf("send desc");
          return -1;
       }
       //eprintf("Sent desc: %s dlen: %lu to %d\n", desc, dlen, ((int *)ctx->out[i]->opaque)[1]);
       if (send_string(ctx->out[i], data, len)) {
          eprintf("send data");
          return -1;
       }
       //eprintf("Sent data with len: %lu to %d\n", len, ((int *)ctx->out[i]->opaque)[1]);
   }

   /*
   if (send_log(ctx)) {
      set_error("send log");
      return -1;
   }
   */

   return 0;
}


/*
 *********** Certification Functions **********
 */
inline
int send_mrenclave(int fd_in, int fd_out)
{
   int ret;
   SGX_report rep;
   SGX_userdata_t userdata __align(128);
   SGX_targetinfo targetinfo __align(128);
   _ereport(&rep, &userdata, &targetinfo);

   /* build temp write only channel */
   channel_t *tmp = build_plain_text_channel(fd_in, fd_out);
   if (!tmp) {
       return -1;
   }
   ret = write_nbytes(tmp, &rep.mrenclave, sizeof(rep.mrenclave));
   free_channel(tmp, 0);
   return ret;
}

inline
int setup_for_cert(WorkSpec *spec)
{
    return setup_for_work(spec);
}

inline
unsigned char *get_result(cert_ctx ctx, int64_t *len)
{
    unsigned char *ret;
    ret = get_work(ctx, len);
    return ret;
}
inline
int get_result_desc(cert_ctx ctx, char **desc, int64_t *dlen,
        unsigned char ** data, int64_t *len)
{
    int ret;
    ret = get_work_desc(ctx, desc, dlen, data, len);
    return ret;
}

inline
void print_log(cert_ctx ctx) {
    dump_log_t(&ctx->log);
}

inline
int check_log(cert_ctx ctx)
{
   if (log_check_macs(&ctx->log)) {
       set_error("bad mac");
       return -1;
   }
   if (log_check_hashes(&ctx->log)) {
       set_error("bad log hash");
       return -1;
   }
   if (data_check_hashes(&ctx->log)) {
       set_error("bad data hash");
       return -1;
   }
   return 0;
}
