#include <unistd.h>
#include <stdlib.h>

#include "request.h"

#include "pipeline/pipeline.h"
#include "pipeline/worker.h"

const char *prog_name = "svmsgd_end_pipeline";

int run_pipeline()
{
  int ret = 0;
  unsigned char *data, *tmp;
  char *desc;
  work_ctx ctx = alloc_ctx();
  int64_t len, dlen;
  health_request_info *info;

  while (1) {
    int r;
    r = wait_for_chan(false, ctx, (uint8_t **)&desc, (uint64_t *)&dlen, (uint8_t **)&data, (uint64_t *)&len);
    if (r) {
      ret = -1;
      break;
    }
#ifdef __native_client__
    r = get_work_desc(ctx, &desc, &dlen, &data, &len);
    if (r == 1)
      continue;
    if (r == -1) {
      ret = -1;
      fprintf(stderr, "error get_work_desc\n");
      break;
    }
#endif
    if (desc == NULL || data == NULL) {
      ret = -1;
      fprintf(stderr, "error desc or data is NULL\n");
      break;
    }
    printf(" ----- %s get desc %s\n", prog_name, desc);
    put_work_desc(ctx, desc, dlen, data, len);
    free(data);
    free(desc);
  }
  return ret;
}

int main(int argc, const char **argv) {
  int ret = 0;
  WorkSpec *spec = WorkSpec_parse(argv[argc - 1]);

  if (spec) {
    assert(spec->n > 0);
    if (setup_for_work(spec)) {
      fprintf(stderr, "setup failed");
      return 1;
    }
  } else {
    fprintf(stderr, "No spec..\n");
    return 1;
  }

  ret = run_pipeline();
  finish_work(1);
  return ret;
}
