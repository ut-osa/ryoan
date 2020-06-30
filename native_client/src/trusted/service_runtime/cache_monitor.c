#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/perf_event.h>
#include <asm/unistd.h>

#include "native_client/src/trusted/service_runtime/cache_monitor.h"

static int miss_fd = -1;
static int inst_fd = -1;
bool enabled = false;

#define INITED (miss_fd >= 0 && inst_fd >= 0)

static long perf_event_open(struct perf_event_attr *hw_event, pid_t pid,
                            int cpu, int group_fd, unsigned long flags)
{
    return syscall(__NR_perf_event_open, hw_event, pid, cpu,
                   group_fd, flags);
}


static inline int init_hw_perf(__u64 config)
{
   int ret;
   struct perf_event_attr pe = {
      .type = PERF_TYPE_HARDWARE,
      .config = config,
      .size = sizeof(struct perf_event_attr),
      .pinned = 1,
      .disabled = 1,
      .exclude_kernel = 1,
      .exclude_hv = 1,
   };

   ret = perf_event_open(&pe, 0, -1, -1, 0);
   if (ret < 0)
      perror("perf_event_open");
   return ret;
}


static inline int enable_perf_fd(int fd)
{
    if (ioctl(fd, PERF_EVENT_IOC_RESET, 0)) {
       perror("PERF_EVENT_IOC_RESET");
       return -1;
    }
    if (ioctl(fd, PERF_EVENT_IOC_ENABLE, 0)) {
       perror("PERF_EVENT_IOC_ENABLE");
       return -1;
    }
    return 0;
}


static inline int disable_perf_fd(int fd)
{
   if (ioctl(fd, PERF_EVENT_IOC_DISABLE, 0)) {
      perror("PERF_EVENT_IOC_DISABLE");
      return -1;
   }
   return 0;
}


void cache_monitor_fini(void)
{
   if (inst_fd > -1)
      close(inst_fd);
   if (miss_fd > -1)
      close(miss_fd);
   inst_fd = miss_fd = -1;
}


int cache_monitor_init(void)
{
   miss_fd = init_hw_perf(PERF_COUNT_HW_CACHE_MISSES);
   if (miss_fd < 0)
      goto err;

   inst_fd = init_hw_perf(PERF_COUNT_HW_INSTRUCTIONS);
   if (inst_fd < 0)
      goto err;

   return 0;

err:
   cache_monitor_fini();
   return -1;
}


int cache_monitor_enable(void)
{
   if (INITED) {
      enabled = true;
      return enable_perf_fd(miss_fd) | enable_perf_fd(inst_fd);
   } else
      return 0;
}


int cache_monitor_disable(void)
{
   if (INITED) {
      enabled = false;
      return disable_perf_fd(miss_fd) | disable_perf_fd(inst_fd);
   } else
      return 0;
}

void cache_monitor_report(FILE *stream)
{
   long long misses, insts;
   bool was_enabled = enabled;

   if (!INITED)
      return;
   cache_monitor_disable();

   if (sizeof(misses) != read(miss_fd, &misses, sizeof(misses)))
      perror("reading misses");
   if (sizeof(insts) != read(inst_fd, &insts, sizeof(insts)))
      perror("reading misses");
   fprintf(stream, "CACHE MISS REPORT: %lld misses for %lld instructions: %.3f "
                   "instructions/miss\n", misses, insts,
                                          ((double)insts)/misses);
   if (was_enabled)
      cache_monitor_enable();
}
