#include <assert.h>
#include <errno.h>
#include <poll.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#ifndef NOT_SGX
#include "enclaveops.h"
#include "sgx.h"
#endif

#include "pipeline/cert.h"
#include "pipeline/channel.h"
#include "pipeline/errors.h"
#include "pipeline/pipeline.h"
#include "pipeline/utils.h"
#include "pipeline/worker.h"
#include "sodium.h"

/* Macros so we can be agnostic to channel type */
#define SETUP_CHANNEL(...) build_d_h_channel(__VA_ARGS__)
#define TEARDOWN_CHANNEL(...) free_channel(__VA_ARGS__)
#define MAX_CHANNELS 64

typedef struct log_entry_t {
  hash_t hash;
  SGX_report report;
} log_entry_t;

typedef struct log_t {
  log_entry_t *first;
  size_t len;
  size_t cap;
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

typedef struct d_h_key_t { unsigned char payload[20]; } d_h_key_t;

uint8_t *nonce = NULL;
uint8_t *key = NULL;

uint8_t *get_nonce(void) {
  if (nonce == NULL) {
    nonce = (uint8_t *)malloc(crypto_aead_aes256gcm_NPUBBYTES);
    if (nonce == NULL) {
      error("Fail to allocate memory for nonce");
      return NULL;
    }
    randombytes_buf(nonce, crypto_aead_aes256gcm_NPUBBYTES);
  } else {
    sodium_increment(nonce, crypto_aead_aes256gcm_NPUBBYTES);
  }
  return nonce;
}

uint8_t *get_fake_key(void) {
  if (key == NULL) {
    key = (uint8_t *)malloc(crypto_aead_aes256gcm_KEYBYTES);
    sodium_hex2bin(key, crypto_aead_aes256gcm_KEYBYTES, FAKE_KEY_HEX,
                   strlen(FAKE_KEY_HEX), NULL, NULL, NULL);
  }
  return key;
}

int read_nbytes(channel_t *chan, void *bytes, size_t len) {
  ssize_t ret;
  size_t _read = 0;
  size_t to_read = len, next_chunk;

  if (!len) {
    return 0;
  }
  /* Have to break this into chunks in order to avoid overflowing the
     syscall buffer */
  do {
    next_chunk = to_read < 4096 ? to_read : 4096;
    ret = chan_read(chan, bytes + _read, next_chunk);
    if (ret < 0) {
      eprintf("chan_read: %s\n", strerror(errno));
      return -1;
    }
    _read += ret;
    to_read -= ret;
  } while (_read < len && ret);

  if (_read != len) {
    /*
    eprintf("Fail to read required bytes\n");
    eprintf("Expected %d bytes\n", (int)len);
    eprintf("Actual read %d bytes\n", (int)_read);
    */
    return -1;
  }

  return 0;
}

int write_nbytes(channel_t *chan, const void *bytes, size_t len) {
  ssize_t ret;
  size_t written = 0;

  if (!len) {
    return 0;
  }

  do {
    ret = chan_write(chan, bytes + written, len - written);
    if (ret < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) continue;
      return -1;
    }
    written += ret;
  } while (written < len);

  if (written != len) {
    return -1;
  }

  return 0;
}

static inline int send_string(channel_t *chan, const void *string, size_t len) {
  int ret = write_nbytes(chan, &len, sizeof(size_t));
  if (ret) return ret;
  return write_nbytes(chan, string, len);
}

struct pipe_ctx *alloc_ctx() {
  struct pipe_ctx *ctx;

  ctx = malloc(sizeof(struct pipe_ctx));
  if (!ctx) {
    set_error("malloc ctx");
    return NULL;
  }

  memset(ctx, 0, sizeof(struct pipe_ctx));

  ctx->nin = pipeline_state.nin;
  ctx->nout = pipeline_state.nout;
  memcpy(ctx->in, pipeline_state.in, sizeof(channel_t) * ctx->nin);
  memcpy(ctx->out, pipeline_state.out, sizeof(channel_t) * ctx->nout);

  return ctx;
}

static inline int read_certifier_id(channel_t *chan, SGX_identity_t *id) {
  return read_nbytes(chan, id, sizeof(*id));
}

/* TODO: Make this Async */
static unsigned char *recv_string_async(channel_t *chan, ssize_t *len) {
  unsigned char *buffer;
  if (read_nbytes(chan, len, sizeof(ssize_t))) {
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

unsigned char *recv_string(channel_t *chan, ssize_t *len) {
  unsigned char *buffer;
  if (read_nbytes(chan, len, sizeof(ssize_t))) {
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

int recv_alloc_log_single(channel_t *in, log_t *log) {
  size_t size;
  int ret = read_nbytes(in, &size, sizeof(size_t));
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
  } else {
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

static int compute_log_hash(hash_t *hash, const log_t *log, size_t n) {
  size_t total_size;
  assert(log->cap >= n);

  total_size = sizeof(log_entry_t) * n;

  /* assume the data hash of n+1 should be involved as well */
  total_size += sizeof(hash_t);
  return compute_hash(hash, log->first, total_size);
}

static int gen_report(struct pipe_ctx *ctx, SGX_report *report) {
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

static int log_update(struct pipe_ctx *ctx, const void *data, size_t len) {
  assert(ctx->log.len < ctx->log.cap);
  log_entry_t *next = ctx->log.first + ctx->log.len;

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

static inline int start_pipeline(struct pipe_ctx *ctx, const void *data,
                                 size_t len) {
  assert(ctx->log.len == 0);
  return log_update(ctx, data, len);
}

void free_ctx(struct pipe_ctx *ctx) {
  /* log entries are always allocated as one big slab */
  if (ctx->log.first) {
    free(ctx->log.first);
  }
  free(ctx);
}

static inline int cmp_hash(hash_t *lhs, hash_t *rhs) {
  return memcmp(lhs, rhs, sizeof(hash_t));
}

int check_in_hash(log_t *log, unsigned char *data, size_t len) {
  hash_t hash;
  if (compute_hash(&hash, data, len)) {
    return -1;
  }
  if (cmp_hash(&hash, &log->first[log->len - 1].hash)) {
    return -1;
  }
  return 0;
}

static inline int send_log_single(channel_t *out, log_t *log) {
  return send_string(out, log->first, log->len * sizeof(log_entry_t));
}

static void dump_report(const SGX_report *report) {
  int i;
  printf("userdata: ");
  /* XXX SGX_userdata_t is an array of uint8_t */
  for (i = 0; i < sizeof(SGX_userdata_t); ++i) {
    printf("%hhx", report->userdata.payload[i]);
  }
  printf("\n");
}

static void dump_hash_t(const hash_t *hash) {
  int i;
  printf("hash: ");
  for (i = 0; i < sizeof(hash_t); ++i) {
    printf("%hhx", hash->payload[i]);
  }
  printf("\n");
}

static void dump_log_entry_t(const log_entry_t *entry) {
  printf("\t");
  dump_hash_t(&entry->hash);
  printf("\t");
  dump_report(&entry->report);
}

static void dump_log_t(const log_t *log) {
  int i;
  printf("entries: %lu\nlog:\n", log->len - 1);
  for (i = 0; i < log->len; ++i) {
    dump_log_entry_t(log->first + i);
  }
}

static int log_check_macs(log_t *log) {
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

static int log_check_hashes(log_t *log) {
  int i = log->len - 1;
  log_entry_t *entry = log->first + i;
  hash_t hash;
  hash_t *stored_hash;

  for (; entry >= log->first; --entry, --i) {
    assert(i >= 0);
    stored_hash = (hash_t *)&entry->report.userdata;
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

static int data_check_hashes(log_t *log) {
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
int feed_data(int top_in, int top_out, const void *data, size_t len,
              Channel_Type type) {
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

int feed_data_desc_chan_no_model(channel_t *top, const void *desc, size_t dlen,
                                 const void *data, size_t len) {
  int ret = 0;
  ret = send_string(top, desc, dlen);
  if (ret) {
    eprintf("send desc error");
    return ret;
  }
  // eprintf("Sent desc: %s and dlen: %d\n", (char *) desc, dlen);

  ret = send_string(top, data, len);
  if (ret) {
    eprintf("send data error");
    return ret;
  }
  // eprintf("Sent data: %s and len: %d\n", (char *) data, len);
  return ret;
}

int feed_data_desc(int top_in, int top_out, const void *desc, size_t dlen,
                   const void *data, size_t len, Channel_Type type,
                   bool send_to_nacl) {
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
  printf("string sent");

  ret = feed_data_desc_chan_no_model(top, desc, dlen, data, len);
  free_channel(top, 0);
  return ret;
}

int feed_data_desc_chan_no_spec(channel_t **top, int n_chans, const void *desc,
                                size_t dlen, const void *data, size_t len,
                                bool enc) {
  int i = 0, ret = 0, pos = 0;
  uint8_t *buf = NULL, *ctext_buf = NULL, *nonce = NULL, *key;
  uint64_t ctext_len;
  for (i = 0; i < n_chans; ++i) {
    size_t total_size = dlen + len + sizeof(size_t) * 2;
    size_t transmit_size = total_size + sizeof(size_t);
    pos = 0;
    buf = (uint8_t *)malloc(transmit_size);
    if (buf == NULL) {
      eprintf("Fail to allocate memory for plaintext");
      return -1;
    }
    // eprintf("Total size is %lu\n", total_size);
    memcpy(buf + pos, &total_size, sizeof(size_t));
    pos += sizeof(size_t);
    memcpy(buf + pos, &dlen, sizeof(size_t));
    pos += sizeof(size_t);
    memcpy(buf + pos, desc, dlen);
    pos += dlen;
    memcpy(buf + pos, &len, sizeof(size_t));
    pos += sizeof(size_t);
    memcpy(buf + pos, data, len);
    if (enc) {
      ctext_buf =
          (uint8_t *)malloc(transmit_size + crypto_aead_aes256gcm_ABYTES);
      if (ctext_buf == NULL) {
        eprintf("Fail to allocate memory for ciphertext");
        return -1;
      }
      nonce = get_nonce();
      if (nonce == NULL) {
        error("Fail to get nonce");
        return -1;
      }
      key = get_fake_key();
      if (key == NULL) {
        error("Fail to get key");
        return -1;
      }
      // encrypt
      crypto_aead_aes256gcm_encrypt(ctext_buf, (unsigned long long *)&ctext_len,
                                    buf, transmit_size, NULL, 0, NULL, nonce,
                                    key);
      transmit_size = ctext_len + crypto_aead_aes256gcm_NPUBBYTES;
    }
    ret = write_nbytes(top[i], &transmit_size, sizeof(size_t));
    if (ret) {
      error("send data");
      return -1;
    }
    if (enc) {
      ret = write_nbytes(top[i], nonce, crypto_aead_aes256gcm_NPUBBYTES);
      if (ret) {
        error("send data");
        return -1;
      }
      ret = write_nbytes(top[i], ctext_buf, ctext_len);
      if (ret) {
        error("send data");
        return -1;
      }
      free(ctext_buf);
    } else {
      ret = write_nbytes(top[i], buf, transmit_size);
      if (ret) {
        error("send data");
        return -1;
      }
    }
    free(buf);
  }
  return ret;
}

int feed_data_desc_chan(channel_t **top, const WorkSpec *spec, const void *desc,
                        size_t dlen, const void *data, size_t len, bool enc) {
  return feed_data_desc_chan_no_spec(top, spec->n, desc, dlen, data, len, enc);
}

/* like feed data but always uses a plaintext channel to start */
int feed_data_plain(int top_in, int top_out, int bottom_in, int bottom_out,
                    const void *data, size_t len) {
  int ret = 0;
  SGX_identity_t mrenclave; /* measurement of the certifier */
  size_t log_len = 0;       /* because we're at the head */
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

  ret = chan_write(top, &log_len, sizeof(size_t));
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
int setup_for_work(WorkSpec *spec) {
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

unsigned char *get_work(work_ctx ctx, ssize_t *len) {
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
  // err_free_data:
  free(data);
  return NULL;
}

static channel_t *sleep_until_ready_common(channel_t **chans, int n) {
  int i;
  int negated = 0;
  struct pollfd fds[n];
  int *fdp[n];
  channel_t *chan = NULL;
  memset(fds, 0, sizeof(struct pollfd) * n);

  for (i = 0; i < n; ++i) {
    fdp[i] = chans[i]->opaque;
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

void sleep_until_ready(work_ctx ctx) {
  sleep_until_ready_common(ctx->in, ctx->nin);
}

int get_chan_data_no_ctx(channel_t **chans, int n_chans, unsigned char **buffer,
                         size_t *size, bool enc) {
  uint8_t *_buffer = NULL;
  size_t _size = 0;
  size_t total_item = 0;
  int i = 0;
  channel_t *chan;

  chan = sleep_until_ready_common(chans, n_chans);
  if (!chan) {
    eprintf("chan_ready: %s\n", strerror(errno));
    return -1;
  }
  if (read_nbytes(chan, &total_item, sizeof(size_t)) < 0) {
    eprintf("read failed\n");
    return -1;
  }
  // eprintf("Total item to get: %lu\n", total_item);
  // TODO: Less memcpy here. Setup a linked list here.
  if (total_item > 0) {
    while (1) {
      size_t transmit_size = 0;
      size_t used_size = 0;
      uint8_t *__buffer = NULL;
      if (read_nbytes(chan, &transmit_size, sizeof(size_t)) < 0) {
        eprintf("Fail to get transmit size\n");
        return -1;
      }
      // eprintf("Transmit size is %lu\n", transmit_size);
      if (enc) {
        uint8_t *ctext_buf = NULL;
        uint8_t nonce[crypto_aead_aes256gcm_NPUBBYTES];
        uint64_t ctext_len = 0, ptext_len = 0;
        // Get nonce
        if (read_nbytes(chan, nonce, crypto_aead_aes256gcm_NPUBBYTES) < 0) {
          eprintf("Fail to read in nonce\n");
          return -1;
        }
        // decrypt
        ctext_len = transmit_size - crypto_aead_aes256gcm_NPUBBYTES;
        if (ctext_len <= 0) {
          eprintf("Wrong ciphertext size\n");
          return -1;
        }
        if (ctext_len < crypto_aead_aes256gcm_ABYTES) {
          eprintf("Message forged\n");
          return -1;
        }
        __buffer = (uint8_t *)malloc(ctext_len);
        if (__buffer == NULL) {
          error("Fail to allocate memory for plaintext");
          return -1;
        }
        ctext_buf = (uint8_t *)malloc(ctext_len);
        if (ctext_buf == NULL) {
          error("Fail to allocate memory for ciphertext");
          return -1;
        }
        if (read_nbytes(chan, ctext_buf, ctext_len) < 0) {
          eprintf("Fail to read in ciphertext\n");
          free(ctext_buf);
          free(__buffer);
          return -1;
        }
        if (crypto_aead_aes256gcm_decrypt(
                __buffer, (unsigned long long *)&ptext_len, NULL, ctext_buf,
                ctext_len, NULL, 0, nonce, get_fake_key()) != 0) {
          eprintf("Message forged!\n");
          free(ctext_buf);
          free(__buffer);
          return -1;
        }
        free(ctext_buf);
      } else {
        __buffer = (uint8_t *)malloc(transmit_size);
        if (__buffer == NULL) {
          error("Fail to allocate memory for data");
          return -1;
        }
        if (read_nbytes(chan, __buffer, transmit_size) < 0) {
          eprintf("Cannot read in data\n");
          free(__buffer);
          return -1;
        }
      }

      used_size = ((uint64_t *)__buffer)[0];
      // eprintf("Used size is 0x%lx(%lu)\n", used_size, used_size);
      if (used_size > 0) {
        if (_buffer == NULL) {
          _buffer = (uint8_t *)malloc(used_size);
          if (_buffer == NULL) {
            eprintf("Fail to allocate memory\n");
            free(__buffer);
            return -1;
          }
          _size = 0;
        } else {
          _buffer = realloc(_buffer, used_size + _size);
        }
        memcpy(_buffer + _size, __buffer + sizeof(uint64_t), used_size);
        _size += used_size;
      }
      i++;
      if (i == total_item) {
        fprintf(stdout, "get all data from previous stage\n");
        break;
      }
      free(__buffer);
    }
  } else if (total_item == 0) {
    return 1;
  }
  if (_buffer == NULL) {
    eprintf("No data available\n");
    return -1;
  } else {
    *buffer = _buffer;
    *size = _size;
    return 0;
  }
}

int get_chan_data(work_ctx ctx, unsigned char **buffer, size_t *size,
                  bool enc) {
  return get_chan_data_no_ctx(ctx->in, ctx->nin, buffer, size, enc);
}

int get_work_desc_buffer(unsigned char *buffer, size_t size, size_t start,
                         size_t *end, char **desc, ssize_t *dlen,
                         unsigned char **data, ssize_t *len) {
  char *_desc = 0;
  unsigned char *_data = 0;
  // eprintf("total size is %lu\n", size);

  if (size - start > sizeof(ssize_t)) {
    memcpy(dlen, buffer + start, sizeof(ssize_t));
    // eprintf("desc size is %d\n", (int)*dlen);
    if (*dlen < size - start - sizeof(ssize_t)) {
      _desc = (char *)malloc(*dlen);
      memcpy(_desc, buffer + start + sizeof(ssize_t), *dlen);
      // eprintf("desc is %s", _desc);
      if (size - start - sizeof(ssize_t) - *dlen > sizeof(ssize_t)) {
        memcpy(len, buffer + start + sizeof *dlen + *dlen, sizeof(ssize_t));
        // eprintf("Get data len: %d\n", (int)*len);
        // eprintf("rest length: %d\n", (int)(size - sizeof(ssize_t)*2 -
        // *dlen));
        if (size - start - sizeof(ssize_t) * 2 - *dlen >= *len) {
          _data = (unsigned char *)malloc(*len);
          memcpy(_data, buffer + start + sizeof(ssize_t) * 2 + *dlen, *len);
          // eprintf("----get all expected data-------\n");
          *data = _data;
          *desc = _desc;
          *end = start + sizeof(ssize_t) * 2 + *dlen + *len;
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

int get_work_desc_no_ctx(channel_t **chans, int n_chans, char **desc,
                         ssize_t *dlen, unsigned char **data, ssize_t *len) {
  char *_desc = NULL;
  unsigned char *_data;
  channel_t *chan;

  /*
     if (read_certifier_id(_ctx->in, &_ctx->certifier)) {
     set_error("read certifier");
     goto err_free_ctx;
     }
     */

  chan = sleep_until_ready_common(chans, n_chans);
  if (!chan) return -1;
  _desc = (char *)recv_string_async(chan, dlen);
  if (!_desc) {
    return -1;
  }

  _data = recv_string_async(chan, len);
  if (!_data) {
    goto err_free_desc;
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
  *data = _data;
  *desc = _desc;
  return 0;
err_free_desc:
  free(_desc);
  return -1;
}

/* Return 1 if no work but some channels un available
 *       -1 if no work and all channels closed
 *       0 otherwise (success)
 */
int get_work_desc(work_ctx ctx, char **desc, ssize_t *dlen,
                  unsigned char **data, ssize_t *len) {
  return get_work_desc_no_ctx(ctx->in, ctx->nin, desc, dlen, data, len);
}

int put_work(work_ctx ctx, const unsigned char *data, size_t len) {
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
int put_work_desc(work_ctx ctx, const char *desc, size_t dlen,
                  const unsigned char *data, size_t len) {
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
    if (send_string(ctx->out[i], data, len)) {
      eprintf("send data");
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

/*
 *********** Certification Functions **********
 */
inline int send_mrenclave(int fd_in, int fd_out) {
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

inline int setup_for_cert(WorkSpec *spec) { return setup_for_work(spec); }

inline unsigned char *get_result(cert_ctx ctx, ssize_t *len) {
  unsigned char *ret;
  ret = get_work(ctx, len);
  return ret;
}
inline int get_result_desc(cert_ctx ctx, char **desc, ssize_t *dlen,
                           unsigned char **data, ssize_t *len) {
  int ret;
  ret = get_work_desc(ctx, desc, dlen, data, len);
  return ret;
}

inline void print_log(cert_ctx ctx) { dump_log_t(&ctx->log); }

inline int check_log(cert_ctx ctx) {
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
