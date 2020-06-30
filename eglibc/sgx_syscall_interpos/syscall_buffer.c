#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/mman.h>
#include "memmgr.h"
#include "shim_table.h"
#include <sgx_runtime.h>
#include <syscall_shim.h>
#include "enclave_mman.h"

bool _in_sgx;
bool in_sgx;
extern void *untrusted_syscall_handler;
extern void *untrusted_error_handler;
extern void *untrusted_clone_handler;

/* This uglyness is because we can't set __tls_size until ldso has relocated
   itself */
extern uint64_t __tls_size; /* this is global to the program */
extern uint64_t _tls_size;  /* this is global but local to this .so */
int thread_priority;
int no_rdrand = 1;

static void init_syscall_buffer(void *buf, unsigned long size)
{
    memmgr_init(buf, size);
}
struct sgx_man_info enclave_params;

#define KB 1024UL
#define MB (KB * 1024UL)
#define GB (MB * 1024UL)
#define SYSCALL_BUFFER_SIZE (4096 * 4096 * 4)
#define MMAP_AREA_SIZE (128 * GB)
#ifndef SHARED
/*XXX a hack so that the glibc statically linked installation programs will run
 * we don't support static executables in enclaves */
unsigned char syscall_buffer[SYSCALL_BUFFER_SIZE];
unsigned long syscall_buffer_size = SYSCALL_BUFFER_SIZE;
void static_sgx_setup(void)
{
    init_syscall_buffer(syscall_buffer, syscall_buffer_size);
}
#endif

static void untrusted_init(struct sgx_man_info *info)
{
    void *mmap_pool;
    void *buffer;
    /* memory area to manage */
    mmap_pool = mmap(NULL, MMAP_AREA_SIZE, PROT_NONE,
                     MAP_ANONYMOUS|MAP_PRIVATE|MAP_UNTRUST, -1, 0);

    if (mmap_pool == MAP_FAILED) {
        DIE("Failed to map mmap_pool");
    }

    /* syscall_buffer */
    buffer = mmap(NULL, SYSCALL_BUFFER_SIZE, PROT_READ|PROT_WRITE,
                  MAP_ANONYMOUS|MAP_PRIVATE|MAP_UNTRUST, -1, 0);

    if (mmap_pool == MAP_FAILED) {
        DIE("Failed to map syscall_buffer");
    }
    memset(info, 0, sizeof(struct sgx_man_info));
    if (PASS_THROUGH_SYSCALL(munmap, 2, mmap_pool, MMAP_AREA_SIZE))
        DIE("MUNMAP failed");

    info->syscall_buffer      = buffer;
    info->syscall_buffer_size = SYSCALL_BUFFER_SIZE;
    info->size                = MMAP_AREA_SIZE + (uint64_t)mmap_pool;
    info->mmap_line           = (uint64_t)mmap_pool;
}


int do_sgx_man_init(uint64_t sgx_check, struct sgx_man_info *info)
{
    if (sgx_check == SGX_KEY) {
        memcpy(&enclave_params, info, sizeof(struct sgx_man_info));
        _in_sgx = in_sgx = true;
        __tls_size = _tls_size;
    } else {
        info = &enclave_params;
        untrusted_init(info);
        /* TODO test for RDRAND */
    }
    init_syscall_buffer(info->syscall_buffer, info->syscall_buffer_size);
    if (init_encl_mman(info->mmap_line, info->size - info->mmap_line))
        DIE("init_encl_mman failed");


    untrusted_syscall_handler = enclave_params.syscall_handler;
    untrusted_error_handler = enclave_params.error_handler;
    untrusted_clone_handler = enclave_params.clone_handler;
    return 0;
}

volatile int exclusion = 0;
void _lock(void) {
    while (__sync_lock_test_and_set(&exclusion, 1)) {
        // Do nothing. This GCC builtin instruction
        // ensures memory barrier.
    }
}
void _unlock(void) {
    __sync_synchronize(); // Memory barrier.
    exclusion = 0;
}



void *get_syscall_buffer(unsigned long size)
{
    void *ret;
    _lock();
    ret = memmgr_alloc(size);
    _unlock();
    return ret;
}

void free_syscall_buffer(void *ptr)
{
    if (ptr) {
        _lock();
        memmgr_free(ptr);
        _unlock();
    }
}

static size_t null_term_arr_len(const void *const *const arr)
{
   size_t i;
   for (i = 0; arr[i]; i++)
      /* nothing */;
   return i;
}

static char *string_buffer(const char* str, size_t *size)
{
   char *result = NULL;
   if (str) {
      *size = (strlen(str) + 1) * sizeof(char);
      result = get_syscall_buffer(*size);
   }
   return result;
}

char *string_buffer_out(const char* str)
{
   size_t size;
   char *result = string_buffer(str, &size);
   if (result)
      memcpy(result, str, size);
   return result;
}

void free_string_buffer(char *str)
{
   free_syscall_buffer(str);
}

void free_string_array_buffer(char **str)
{
   char **tmp;
   if (!str)
      return;
   for (tmp = str; *tmp; tmp++)
      free_string_buffer(*tmp);
   free_syscall_buffer(str);
}

char **string_array_buffer_out(const char *const arr[])
{
   char **result = NULL;
   int i;
   size_t size = (null_term_arr_len(arr) + 1) * sizeof(char *);
   result = get_syscall_buffer(size);
   if (!result)
      return NULL;
   for (i = 0; arr[i]; i++) {
      result[i] = string_buffer_out(arr[i]);
      if (! result[i]) {
         free_string_array_buffer(result);
         return NULL;
      }
   }
   result[i] = NULL;

   return result;
}


#if 0
static BuffMan buffer;
static inline
void init_syscall_buffer()
{
    assert(syscall_buffer);
    buff_man_init(&buffer, syscall_buffer, syscall_buffer_size);
}

void *get_syscall_buffer(unsigned long size)
{
    if (!buffer.base) {
        init_syscall_buffer();
    }
    if (!size) {
        return NULL;
    }
    return buff_man_alloc(&buffer, size);
}

void free_syscall_buffer(void *ptr)
{
    if (!buffer.base) {
        init_syscall_buffer();
    }
    if (!ptr) {
        return;
    }
    buff_man_free(&buffer, ptr);
}
#endif
