#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <openssl/dh.h>
#include <openssl/err.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <semaphore.h>
#include <fcntl.h>
#include <poll.h>

#include "pipeline/channel.h"
#include "pipeline/errors.h"
#include "pipeline/utils.h"

const char * Channel_Type_str [] = {
    "PLAIN_CHANNEL",
    "DH_CHANNEL",
};
const char * Flow_Dir_str [] = {
    "IN",
    "OUT",
};

static inline int __test(const char *str, const char *arr[], size_t size)
{
    int i;
    for (i = 0; i < size; ++i) {
        if (!strcmp(arr[i], str)) return i;
    }
    return -1;
}

inline const char *flow_dir_str(Flow_Dir type)
{
    return Flow_Dir_str[type];
}

inline const char *chan_type_str(Channel_Type type)
{
    return Channel_Type_str[type];
}
inline Channel_Type str_chan_type(const char* str)
{
    return (Channel_Type)__test(str, Channel_Type_str,
            sizeof(Channel_Type_str) / sizeof(char *));
}
inline Flow_Dir str_flow_dir(const char *str)
{
    return (Flow_Dir)__test(str, Flow_Dir_str,
            sizeof(Flow_Dir_str) / sizeof(char *));
}
/***** GENERAL Channel Helpers ***********/
static
void _free_channel(channel_t* chan) {
    free(chan);
}

static
channel_t *build_channel(chan_read_op chan_read, chan_write_op chan_write,
        chan_free_op chan_free, chan_ready_op chan_ready) {
    channel_t *chan = malloc(sizeof(channel_t));
    if (!chan) {
        return NULL;
    }
    *(chan_read_op *)&chan->read = chan_read;
    *(chan_write_op *)&chan->write = chan_write;
    *(chan_free_op *)&chan->free = chan_free;
    *(chan_ready_op *)&chan->ready = chan_ready;
    chan->opaque = NULL;

    return chan;
}

/********* plaintext channels ********/
static
ssize_t plain_text_read(channel_t* self, void *buff, size_t count)
{
    return read(((int *)self->opaque)[0], buff, count);
}

static
ssize_t plain_text_write(channel_t* self, const void *buff, size_t count)
{
    return write(((int *)self->opaque)[1], buff, count);
}

void free_plain_text_channel(channel_t *chan, int _close)
{
    if (_close) {
        close(((int *)chan->opaque)[0]);
        close(((int *)chan->opaque)[1]);
    }
    /* no special teardown required just DEALLOC THE THING */
    _free_channel(chan);
}

static
int plain_text_ready(channel_t* self, Flow_Dir dir)
{
    struct pollfd fds = {0};

    switch (dir) {
        case IN:
            fds.fd = ((int *)self->opaque)[0];
            fds.events |= POLLIN;
            break;
        case OUT:
            fds.fd = ((int *)self->opaque)[1];
            fds.events |= POLLOUT;
            break;
        default:
            abort();

    }
    return poll(&fds, 1, 0);
}


channel_t *build_plain_text_channel(int in, int out)
{
    /* allocate channel */
    int *fds;
    channel_t *chan = build_channel(plain_text_read, plain_text_write,
            free_plain_text_channel, plain_text_ready);

    if (!chan) {
        return NULL;
    }

    chan->opaque = malloc(sizeof(int) * 2);
    fds = (int *)chan->opaque;
    fds[0] = in;
    fds[1] = out;

    return chan;
}

/**************** D_H (Diffie Hellman) channels******************/
/*auto generated*/
#ifndef HEADER_DH_H
#include <openssl/dh.h>
#endif
DH *get_dh2048()
{
    static unsigned char dh2048_p[]={
        0xDA,0x6A,0xC1,0xD9,0x3B,0xAA,0x85,0x29,0x15,0xC2,0x09,0x20,
        0xF8,0x50,0x5F,0x9D,0x96,0xE7,0xAD,0x32,0xE6,0x69,0xED,0x86,
        0x3B,0xEC,0x89,0xA2,0x43,0x5F,0x1D,0xA2,0x4C,0xCA,0x5A,0x43,
        0x03,0xB1,0x30,0x19,0x35,0x51,0x25,0x31,0xEA,0x78,0xDD,0xF3,
        0xB5,0xB8,0x1E,0x44,0x4D,0x71,0x97,0xD5,0x8C,0x9D,0xA0,0xE4,
        0x84,0xAF,0x81,0xBE,0xA5,0xDB,0x06,0x77,0x1C,0xEE,0xE1,0xF6,
        0x0C,0x93,0x47,0x74,0x7A,0xC8,0x87,0xBF,0x97,0xF6,0x17,0x96,
        0xB0,0x31,0xBE,0x11,0x21,0x9E,0x82,0x74,0x98,0xBC,0x26,0x21,
        0xFD,0x22,0x42,0x42,0xD4,0x9B,0xC6,0x51,0x77,0x7A,0x65,0xC5,
        0x1B,0x6D,0x6D,0x00,0xCB,0xD4,0x57,0xBB,0xC6,0x0F,0x4E,0xEC,
        0x4F,0x9A,0x55,0xE8,0xDD,0x7F,0xF7,0x47,0xFC,0x03,0x52,0x92,
        0xCA,0xFA,0x6F,0x54,0x7B,0x3E,0xA3,0x3C,0xE1,0x21,0x96,0x8F,
        0xCE,0x6D,0x28,0xA9,0xAE,0x93,0x18,0xAF,0x95,0x39,0x82,0xAF,
        0xA9,0x2C,0x06,0xD9,0x0F,0x07,0x83,0xEC,0xBC,0xF9,0xFD,0x44,
        0x55,0x31,0x27,0x17,0x2B,0x87,0x14,0x06,0x74,0xCB,0x0E,0x72,
        0x6E,0x38,0xD8,0x11,0x97,0x35,0x25,0xCA,0x5E,0xCC,0xE4,0xAD,
        0xF3,0x31,0xDF,0xBE,0x4B,0x64,0x5F,0x21,0xD6,0x29,0xAD,0xD0,
        0xC8,0x79,0x9F,0x73,0x16,0x24,0x6E,0x72,0x8E,0xD3,0xB1,0x83,
        0xC8,0x59,0xEB,0xCA,0xDF,0xDB,0xB0,0x8C,0x5B,0x71,0xAE,0xFC,
        0x5F,0x54,0x1B,0x37,0xB2,0xB8,0xE0,0xFC,0xCD,0xD9,0x13,0x74,
        0x6A,0x76,0x62,0xE5,0x9A,0x51,0x56,0x4D,0x97,0x72,0xDB,0xE6,
        0xF7,0xAB,0xFA,0xD3,
    };
    static unsigned char dh2048_g[]={
        0x02,
    };
    DH *dh;

    if ((dh=DH_new()) == NULL)
        return(NULL);

    dh->p=BN_bin2bn(dh2048_p,sizeof(dh2048_p),NULL);
    dh->g=BN_bin2bn(dh2048_g,sizeof(dh2048_g),NULL);

    if ((dh->p == NULL) || (dh->g == NULL)) {
        DH_free(dh);
        return(NULL);
    }
    return(dh);
}


static
DH *create_dh()
{
    DH *dh = get_dh2048();
    if (!dh)
        return NULL;

    if (DH_generate_key(dh) != 1) {
        char buf[100];
        eprintf("%s", ERR_error_string(ERR_get_error(), buf));
        eprintf("could not generate key\n");
        DH_free(dh);
        return NULL;
    }
    return dh;
}

typedef struct dh_context {
    BIO *in;
    BIO *out;
    unsigned char *key;
    SGX_identity_t *other_id;
} dh_context;

#if defined(__native_client__) && defined(_NEWLIB_VERSION)
_Static_assert(sizeof(hash_t) < sizeof(SGX_userdata_t),
      "Chosen hash size takes more bytes than we have");
#endif

static
ssize_t d_h_read(channel_t* self, void *buff, size_t count)
{
    ssize_t ret = BIO_read(((dh_context *)self->opaque)->in, buff, count);
    if (!BIO_get_cipher_status(((dh_context *)self->opaque)->in)) {
        eprintf("decrypt failure\n");
        return -1;
    }
    return ret;
}

static
ssize_t d_h_write(channel_t* self, const void *buff, size_t count)
{
    BIO* out = ((dh_context *)self->opaque)->out;
    ssize_t ret = BIO_write(out, buff, count);
    return ret;
}

static
int hash_report_send(int fd, const void *buffer, size_t size,
        SGX_identity_t *other_id)
{
    SGX_userdata_t userdata __align(128);
    SGX_targetinfo targetinfo __align(128) = {
        .measurement = *other_id,
    };
    SGX_report report;
    memset(&userdata, 0, sizeof(SGX_userdata_t));

    if (compute_hash((hash_t *)&userdata, buffer, size)) {
        return -1;
    }

    if (_ereport(&report, &userdata, &targetinfo)) {
        return -1;
    }

    if (write(fd, &report, sizeof(SGX_report)) != sizeof(SGX_report)) {
        return -1;
    }
    return 0;
}

static
int recieve_report_verify(int fd, const void *buffer, size_t size)
{
    hash_t hash;
    SGX_report report;
    SGX_key_t key __align(16);

    if (compute_hash(&hash, buffer, size)) {
        return -1;
    }
    if (_egetkey(&key)) {
        return -1;
    }
    if (read(fd, &report, sizeof(SGX_report)) != sizeof(SGX_report)) {
        return -1;
    }
    if (verify_mac(&report)) {
        return -1;
    }
    if (memcmp(&hash, &report.userdata, sizeof(hash_t)) != 0) {
        return -1;
    }
    return 0;
}


static
unsigned char *do_dh_key_exchange(int in, int out, SGX_identity_t *other_id)
{
    DH *dh = NULL;
    BIGNUM *other_pub_key= NULL;
    ssize_t key_size = 0;
    unsigned char *buffer = NULL;
    unsigned char *key = NULL;
    ssize_t ret = 0;

    dh = create_dh();
    if (!dh) {
        eprintf("problem initdh\n");
        return NULL;
    }


    key_size = BN_num_bytes(dh->pub_key);

    buffer = malloc(key_size);
    if (!buffer) {
        eprintf("malloc buffer\n");
        goto free_pub_out;
    }

    /* send my public key */
    do {
        ret = write(out, &key_size, sizeof(size_t));
        assert(ret >= 0);
    } while(!ret);
    if (ret != sizeof(size_t)) {
        eprintf("unexpected write\n");
        goto free_buffer_out;
    }

    if (BN_bn2bin(dh->pub_key, buffer) != key_size) {
        eprintf("bn2bin failed\n");
        goto free_buffer_out;
    }
    do {
        ret = write(out, buffer, key_size);
        assert(ret >= 0);
    } while(!ret);

    if (ret != key_size) {
        eprintf("wrote bad public key\n");
        goto free_buffer_out;
    }
    if (hash_report_send(out, buffer, key_size, other_id)) {
        eprintf("couldn't hash and send pub_key\n");
        goto free_buffer_out;
    }

    /* recieve thier public key */
    do {
        ret = read(in, &key_size, sizeof(size_t));
        assert(ret >= 0);
    } while(!ret);
    if (ret != sizeof(size_t)) {
        eprintf("unexpected read\n");
        goto free_buffer_out;
    }

    free(buffer);
    buffer = malloc(key_size);
    if (!buffer) {
        eprintf("malloc buffer\n");
        goto free_dh_out;
    }

    do {
        ret = read(in, buffer, key_size);
        assert(ret >= 0);
    } while(!ret);
    if (ret != key_size) {
        eprintf("read bad public key\n");
        goto free_buffer_out;
    }
    if (recieve_report_verify(in, buffer, key_size)) {
        eprintf("public key verification failed\n");
        goto free_buffer_out;
    }

    if (!(other_pub_key = BN_bin2bn(buffer, key_size, NULL))) {
        eprintf("problem w/received public key\n");
        goto free_buffer_out;
    }

    key = malloc(DH_size(dh));
    if (!key) {
        eprintf("could not allocate key\n");
        goto free_buffer_out;
    }

    if (BN_bn2bin(other_pub_key, buffer) != key_size) {
        eprintf("bn2bin failed\n");
        goto free_key_out;
    }

    key_size = DH_compute_key(key, other_pub_key, dh);
    if (key_size < 0) {
        eprintf("could not compute shared key\n");
        goto free_key_out;
    }

    free(buffer);
    BN_free(other_pub_key);
    DH_free(dh);

    return key;

 free_key_out:
    free(key);
 free_buffer_out:
    free(buffer);
 free_pub_out:
    BN_free(other_pub_key);
 free_dh_out:
    DH_free(dh);
    return NULL;
}

static
int exchange_mrenclave(int in, int out, SGX_identity_t *id)
{
    SGX_userdata_t userdata __align(128);
    SGX_targetinfo targetinfo __align(128);
    SGX_report report;
    if (_ereport(&report, &userdata, &targetinfo))
        return -1;

    /*send ours*/
    if (write(out, &report.mrenclave, sizeof(report.mrenclave)) !=
            sizeof(report.mrenclave))
        return -1;

    /*recieve theirs*/
    if (read(in, id, sizeof(SGX_identity_t)) != sizeof(SGX_identity_t))
        return -1;

    return 0;
}

static
void free_d_h_channel(channel_t *chan, int close)
{
    BIO* out = ((dh_context *)chan->opaque)->out;
    BIO* in = ((dh_context *)chan->opaque)->in;
    unsigned char *key = ((dh_context *)chan->opaque)->key;
    SGX_identity_t *other_id = ((dh_context *)chan->opaque)->other_id;
    if (BIO_flush(out) != 1) {
        eprintf("flush failure\n");
    }
    BIO_free_all(out);
    BIO_free_all(in);
    free(key);
    free(other_id);
    _free_channel(chan);
}

static
int dh_ready(channel_t *self, Flow_Dir dir)
{
    assert(0 && "dh_ready NOT IMPLEMENTED!");
    return 0;
}


channel_t *build_d_h_channel(int in, int out)
{
    unsigned char *key;
    unsigned char *iv;
    SGX_identity_t *other_id;
    dh_context *context;

    channel_t *chan = build_channel(d_h_read, d_h_write, free_d_h_channel,
            dh_ready);
    if (!chan) {
        return NULL;
    }

    other_id = malloc(sizeof(SGX_identity_t));
    if (!other_id) {
        goto free_chan_out;
    }

    if (exchange_mrenclave(in, out, other_id)) {
        goto free_id_out;
    }

    key = do_dh_key_exchange(in, out, other_id);
    if (!key) {
        goto free_id_out;
    }

    /*FIXME should use random IV for each message and send it*/
    iv = malloc(16);
    memset(iv, 0, 16);

    BIO *in_dec_bio = BIO_new(BIO_f_cipher());
    BIO *out_enc_bio = BIO_new(BIO_f_cipher());

    BIO_set_cipher(in_dec_bio,  EVP_aes_128_cbc(), key, iv, 0);
    BIO_set_cipher(out_enc_bio, EVP_aes_128_cbc(), key, iv, 1);

    chan->opaque = malloc(sizeof(dh_context));
    context = (dh_context *)chan->opaque;
    context->in = BIO_push(in_dec_bio, BIO_new_fd(in, BIO_CLOSE));
    context->out = BIO_push(out_enc_bio, BIO_new_fd(out, BIO_CLOSE));
    context->key = key;
    context->other_id = other_id;

    return chan;

 free_id_out:
    free(other_id);
 free_chan_out:
    _free_channel(chan);
    return NULL;
}

/* do switchign for them */
channel_t *build_channel_type(int in, int out, Channel_Type type)
{
    switch (type) {
        case PLAIN_CHANNEL:
            return build_plain_text_channel(in, out);
        case DH_CHANNEL:
            return build_d_h_channel(in, out);
        case UNDEF_CHANNEL:
        default:
            set_error("Unknown channel type");
    }
    return NULL;
}

channel_t *build_channel_desc(ChannelDesc *desc)
{
    switch (desc->type) {
        case PLAIN_CHANNEL:
            return build_plain_text_channel(desc->in_fd, desc->out_fd);
        case DH_CHANNEL:
            return build_d_h_channel(desc->in_fd, desc->out_fd);
        case UNDEF_CHANNEL:
        default:
            set_error("Unknown channel type");
    }
    return NULL;
}
