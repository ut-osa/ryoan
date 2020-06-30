/*
 * Application level Copy On Write
 *
 */
#include "native_client/src/trusted/service_runtime/cow.h"
#include "native_client/src/trusted/service_runtime/malloc_pool.h"

#include <sys/mman.h>
#include <string.h>

#define ROUND_DOWN(a) (a & ~(NACL_PAGESIZE - 1UL))

#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"
#include "native_client/src/trusted/service_runtime/sel_mem.h"
#include "native_client/src/trusted/service_runtime/hashtable.h"
#include "native_client/src/trusted/service_runtime/thread_suspension.h"
#include "native_client/src/trusted/service_runtime/nacl_app_thread.h"
#include "native_client/src/trusted/service_runtime/sys_filename.h"
#include "native_client/src/include/nacl_assert.h"
#include "native_client/src/trusted/service_runtime/usec_wait.h"
#include "native_client/src/trusted/service_runtime/sgx_perf.h"

#define LOCKED_AREA_SIZE (1024UL*1024UL*1024UL*4) // 4GB
#define KB 1024ULL

static clock_t global_clock;
uintptr_t loaded_pages;
uintptr_t inited_pages;

static size_t shm_size;
static
void count_pages_visitor(void *_unused, struct NaClVmmapEntry *entry)
{
   (void)_unused;
   if (!(entry->removed) && entry->prot != PROT_NONE)
      loaded_pages += entry->npages;
}

void measure_size(struct NaClApp *nap)
{
   NaClVmmapVisit(&nap->mem_map, count_pages_visitor, NULL);
}
void note_shm_size(size_t pgs)
{
   shm_size =pgs;
}

struct timeval times[8];
struct ThreadStateSnapshot {
   struct NaClSignalContext *states;
   int num;  /* number size of array*/
};

struct cow_page_entry {
   void *storage;
   void *addr;
   struct addr_range     *region;
   struct cow_page_entry *next;
};

struct CowState {
   struct cow_page_entry *dirty_head;
   cfuhash_table_t *ht;
   struct ThreadStateSnapshot threads;
};
cfuhash_table_t *to_clean;


static
void free_fail(void *unused)
{
   NaClLog(LOG_FATAL, "Tried to free something from ht (should not happen)\n");
   free(unused);
}

static
struct CowState *CowState_ctor(void)
{
   struct CowState *ret = malloc(sizeof(struct CowState));
   if (ret) {
      ret->ht = cfuhash_new_with_free_fn(free_fail);
      ret->dirty_head = NULL;
      if (!ret->ht) {
         free(ret);
         ret = NULL;
      } else {
         /* extra hash table init if needed */
      }
   }
   return ret;
}

static inline
bool only_zero(uint64_t *begin, void *end)
{
   for (; begin < (uint64_t *)end; begin++) {
      if (*begin) return false;
   }
   return true;
}

static inline
struct cow_page_entry *new_storage_page(void *addr, struct addr_range *region)
{
   struct cow_page_entry *entry = malloc(sizeof(struct cow_page_entry));
   if (!entry)
      NaClLog(LOG_FATAL, "Malloc failed (cow entry)\n");
   entry->addr = addr;
   entry->region = region;

   if (!only_zero(addr, (uint8_t *)addr + NACL_PAGESIZE))
   {
      entry->storage = malloc(NACL_PAGESIZE);
      if (!entry->storage)
         NaClLog(LOG_FATAL, "Malloc failed (cow storage entry)\n");
      memcpy(entry->storage, addr, NACL_PAGESIZE);
   } else {
      entry->storage = NULL;
   }

   return entry;
}

struct addr_range {
   uintptr_t begin;
   uintptr_t end;
};
struct addr_ranges {
   struct DynArray r;
   uintptr_t n;
};


struct addr_range *grow_regions(uintptr_t addr, uintptr_t size)
{
   static struct addr_ranges ranges;
   static int inited = 0;
   struct addr_range *ret;
   if (!inited) {
      if (!DynArrayCtor(&ranges.r, 100)) {
         NaClLog(LOG_FATAL, "ERROR init ragnes\n");
      }
      inited = 1;
      ranges.n = 0;
   }
   ret = DynArrayGet(&ranges.r, ranges.n);
   if (!ret) {
      ret = malloc(sizeof(struct addr_range));
      ret->begin = addr;
      ret->end   = addr + size;
   } else if (ret && ret->end == addr) {
      ret->end = addr + size;
   } else {
      ASSERT(addr > ret->end);
      ret = malloc(sizeof(struct addr_range));
      ret->begin = addr;
      ret->end   = addr + size;
      ranges.n++;
      if (!DynArraySet(&ranges.r, ranges.n, ret)) {
         NaClLog(LOG_FATAL, "ERROR dynarray set\n");
      }
   }
   return ret;
}

   static
void populate_cow_state(struct NaClApp *nap, struct NaClVmmapEntry *entry)
{
   struct cow_page_entry *new_entry;
   struct addr_range *region;
   void *old;
   uintptr_t size = entry->npages << NACL_PAGESHIFT;
   uint8_t *addr = (void *)NaClUserToSysAddrNullOkay(nap,
         entry->page_num << NACL_PAGESHIFT);
   uint8_t *last = addr + size;
   if (entry->removed || !(entry->prot & PROT_WRITE)) {
      return;
   }
   ASSERT(!(entry->prot & PROT_EXEC));
   ASSERT((entry->prot & PROT_READ));
   region = grow_regions((uintptr_t)addr, size);

   for (; addr < last; addr += NACL_PAGESIZE) {
      new_entry = new_storage_page(addr, region);
      cfuhash_put_data(nap->cow->ht, &addr, sizeof(void *), new_entry, 0, &old);
      ASSERT(old == NULL);
   }
}


static
void protect_cow_visitor(void *_nap, struct NaClVmmapEntry *entry)
{
   struct NaClApp *nap = _nap;
   uintptr_t addr;
   size_t size;
   if (entry->prot != PROT_NONE)
      inited_pages += entry->npages;


   addr = NaClUserToSysAddrNullOkay(nap, entry->page_num << NACL_PAGESHIFT);

   size = entry->npages << NACL_PAGESHIFT;
   if (mprotect((void *)addr, size, entry->prot & ~PROT_WRITE)) {
      NaClLog(LOG_FATAL, "protect_cow_visitor mprotect failed\n");
   }
}

   static
void start_cow_visitor(void *_nap, struct NaClVmmapEntry *entry)
{
   populate_cow_state((struct NaClApp *)_nap, entry);
}


uintptr_t memset_total;
uintptr_t memcpy_total;
void restore_page(struct cow_page_entry *entry) {
   if (entry->storage) {
      memcpy_total += NACL_PAGESIZE;
      memcpy(entry->addr, entry->storage, NACL_PAGESIZE);
   } else {
      memset_total += NACL_PAGESIZE;
      memset(entry->addr, 0, NACL_PAGESIZE);
   }
   /* One for Eaccept, One for Emodpr */
   emodpe_wait();
   emodpe_wait();
}

uintptr_t mprotect_count;
uintptr_t page_count;
static pthread_mutex_t mcpy_work_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t mcpy_work_cond = PTHREAD_COND_INITIALIZER;

static pthread_mutex_t done_work_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t done_work_cond = PTHREAD_COND_INITIALIZER;

struct cow_page_entry *get_memcpy_work(struct CowState *cow)
{
   struct cow_page_entry *head, *next;
   do {
      head = cow->dirty_head;
      if (!head) break;
      next = head->next;
   } while(!__sync_bool_compare_and_swap(&cow->dirty_head, head, next));
   return head;
}

static int n_active;
static bool hold_mcpy = true;
static bool not_done = true;
void memcpy_worker(void *_cow)
{
   struct CowState *cow = _cow;
   struct cow_page_entry *entry;

   while (1) {
      pthread_mutex_lock(&mcpy_work_mutex);
      while (hold_mcpy)
         pthread_cond_wait(&mcpy_work_cond, &mcpy_work_mutex);
      n_active++;
      pthread_mutex_unlock(&mcpy_work_mutex);

      while((entry = get_memcpy_work(cow))) {
         restore_page(entry);
      }
      hold_mcpy = true;
      pthread_mutex_lock(&done_work_mutex);
      if (--n_active == 0) {
         not_done = false;
         pthread_cond_broadcast(&done_work_cond);
      }
      pthread_mutex_unlock(&done_work_mutex);
   }
}

static
int make_vm_cow_visitor(void *key, uint32_t key_size, void *data,
      uint32_t data_size, void *arg)
{
   struct addr_range *region;
   size_t size;

   (void)(key_size);
   (void)(data);
   (void)(data_size);
   (void)(arg);

   memcpy(&region, key, key_size);
   size = region->end - region->begin;

   mprotect_count++;
   mprotect((void *)region->begin, size, PROT_READ);
   return 1;
}

uintptr_t max_mprotects, max_pages, total_fault_protects = 0;

static inline
void revert_mem_state(struct NaClApp *nap)
{
    struct cow_page_entry *entry;
    page_count = 0;
    memset_total = 0;
    memcpy_total = 0;

    gettimeofday(times, NULL);

    entry = nap->cow->dirty_head;
    while(entry) {
       restore_page(entry);
       page_count++;
       entry = entry->next;
    }


    gettimeofday(times+1, NULL);
    nap->cow->dirty_head = NULL;
    mprotect_count = 0;
    cfuhash_foreach_remove(to_clean, make_vm_cow_visitor, NULL, nap);
    gettimeofday(times+2, NULL);
    max_mprotects = mprotect_count > max_mprotects ? mprotect_count : max_mprotects;
    max_pages = page_count > max_pages ? page_count : max_pages;
}

static inline
void save_thread_state(struct NaClApp *nap, struct ThreadStateSnapshot *snap)
{
    struct NaClAppThread *thread;
    int i;

    snap->num = nap->num_threads;
    snap->states = malloc(sizeof(struct NaClSignalContext) * snap->num);
    if (!snap->states) {
        NaClLog(LOG_FATAL, "ERROR construction threat_state_snapshot");
    }
    for (i = 0; i < snap->num; ++i) {
        thread = DynArrayGet(&nap->threads, i);
        ASSERT(thread);

        NaClAppThreadGetSuspendedRegisters(thread, snap->states + i);
    }
}

static inline
void revert_thread_state(struct NaClApp *nap, struct ThreadStateSnapshot *snap)
{
    struct NaClAppThread *thread;
    int i;

    ASSERT(snap->states);
    for (i = 0; i < snap->num; ++i) {
        thread = DynArrayGet(&nap->threads, i);
        ASSERT(thread);
        NaClAppThreadSetSuspendedRegisters(thread, snap->states + i);
    }
}
static inline
double sub_times(struct timeval *high, struct timeval *low)
{
   double high_res = (double)high->tv_sec + ((double)high->tv_usec / 1000000.0);
   double low_res = (double)low->tv_sec + ((double)low->tv_usec / 1000000.0);

   return high_res - low_res;
}

static struct timeval stop_mode_clock;
void stop_mode_clock_start()
{
   gettimeofday(&stop_mode_clock, NULL);
}

void stop_mode_clock_stop(unsigned long stopmode)
{
   struct timeval stopped;
   gettimeofday(&stopped, NULL);
   printf("(stop point %lu)STOPPED in %.3fsecs\n",
         stopmode, sub_times(&stopped, &stop_mode_clock));
}


unsigned long round_counter = 0;
void start_cow(struct NaClApp *nap)
{
    static unsigned long requests_until_next_reset;
    char buf[128] = {0};
    NaClUntrustedThreadsSuspendAll(nap, 1);
     /* print flush */

     if (++round_counter >= nap->out_iterations) {
         double cputime = ((double)(clock() - global_clock)) / CLOCKS_PER_SEC;
        if (nap->sgx_perf)
           sgx_perf_snprint(buf, 128);
         printf("pid: %d, mprotects %lu, max_pages %lu, total_fault_protects %lu\n",
                getpid(), max_mprotects, max_pages, total_fault_protects);
         printf("round_counter %lu, %s\n"
                "CURRENT CPU TIME %.3f\n", round_counter,  buf, cputime);
     }
    if (!nap->cow) {
        nap->cow = CowState_ctor();
        to_clean = cfuhash_new();
        /*Allocate pool to service app requests for memory while confined*/
        if (NaClInitMallocPool(nap)) {
            NaClLog(LOG_FATAL, "ERROR constructing sgx_malloc_pool\n");
        }

        NaClVmmapVisit(&nap->mem_map, start_cow_visitor, nap);
        /* HACK, don't double manage this area; tell libc hands off*/
        syscall(328, nap->mem_start, LOCKED_AREA_SIZE);

        if (nap->requests_per_reset) {
           NaClVmmapVisit(&nap->mem_map, protect_cow_visitor, nap);

           save_thread_state(nap, &nap->cow->threads);

           requests_until_next_reset = nap->requests_per_reset;
        }
        printf("%d initial_size %lu pages, first_checkpoint_size %lu pages\n", getpid(),loaded_pages - shm_size,
                                                                inited_pages - shm_size - ((1024 *1024)/4));
        fprintf(stderr, "-----------CHECKPOINT---------\n");
        fflush(stderr);
        if (nap->ryoan_stop_mode == 2) {
           stop_mode_clock_stop(2);
           exit(1);
        }

        /* enable flush */
        if (nap->sgx_perf)
           sgx_perf_enable();
    } else {
       static double cur_total;
       static double max_step;
       double step;
        if (requests_until_next_reset == 0) {
            gettimeofday(times+4, NULL);
            NaClClearMallocPool(nap);
            revert_mem_state(nap);
            //revert_thread_state(nap, &nap->cow->threads);

            /* remove all the things! */
            NaClSysRmdir_internal(nap, ":mem:");

            requests_until_next_reset = nap->requests_per_reset;
            gettimeofday(times+5, NULL);

            step = sub_times(times+5, times+4);
            max_step =  step > max_step ? step : max_step;
            cur_total += step;
            NaClLog(LOG_ERROR,"XXXXXXXX\nmemcpy: %.6f\n mprotect %.6f\n"
                  "cur_total %.6f, this step %.6f max_step %.6f\n", sub_times(times + 1, times),
                  sub_times(times + 2, times + 1), cur_total, step, max_step);
        }
    }
    requests_until_next_reset--;

    NaClUntrustedThreadsResumeAll(nap);
}

void start_clock()
{
   static int inited = 0;
   if (inited) return;
   global_clock = clock();
   inited = 1;
}

bool mark_dirty(struct CowState *cow, uintptr_t addr, int *prot)
{
    struct cow_page_entry *cow_entry;
    void *trash;
    if (!cow) {
       /* no cow state so can't recover */
       return false;
    }
    if (cfuhash_get_data(cow->ht, &addr, sizeof(void *),
                         (void **)&cow_entry, NULL)) {
       cow_entry->next = cow->dirty_head;
       cow->dirty_head = cow_entry;

       cfuhash_put_data(to_clean, &cow_entry->region, sizeof(uintptr_t), &addr,
                        0, &trash);

       *prot = PROT_READ|PROT_WRITE;
       return true;
    } else {
       return false;
    }
}

bool try_cow_recover(struct NaClApp *nap, uintptr_t addr)
{
    int prot;
    addr = ROUND_DOWN(addr);
    emodpe_wait();
    eresume_wait();
    if (!mark_dirty(nap->cow, addr, &prot))
    {
       return false;
    }

    total_fault_protects++;
    if (mprotect((void *)addr, NACL_PAGESIZE, prot)) {
        NaClLog(LOG_FATAL, "try_cow_recover mprotect failed 0x%lx\n", addr);
    }
    return true;
}
