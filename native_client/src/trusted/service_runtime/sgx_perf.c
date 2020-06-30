#include "native_client/src/trusted/service_runtime/sgx_perf.h"
#include <unistd.h>

static int enabled;
void sgx_perf_enable(void)
{
  char buf[128];

  if (enabled)
     return;
  if (read(-100, buf, 1)) {
     /* don't care */
  }
  enabled = 1;
  fprintf(stderr, "XXXX SGX PERF ENABLED XXX\n");
}

int sgx_perf_snprint(char *buf, size_t size)
{
   int ret;

   if (!enabled)
      return 0;
   ret = read(-100, buf, size);
   if (ret < 0) {
      buf[0] = '\0';
      perror("read sgx_perf");
      return 0;
   }
   return ret;
}

int sgx_perf_fprint(FILE *fd)
{
   char buf[128];
   int ret;

   ret = sgx_perf_snprint(buf, 128);
   if (ret)
      ret = fprintf(fd, "%s", buf);
   return ret;
}

int sgx_perf_print(void)
{
   return sgx_perf_fprint(stdout);
}
