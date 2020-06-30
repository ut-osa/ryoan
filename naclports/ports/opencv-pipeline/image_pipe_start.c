#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdbool.h>
#include <errno.h>

#ifdef __native_client__
#include <request.h>
#endif

#include "pipeline/pipeline.h"
#include "pipeline/worker.h"
#include "pipeline/json-builder.h"
#include "pipeline/json.h"

const char *prog_name = "image_pipe_begin";

int run_pipeline(bool output_app_label)
{
    unsigned char *data;
    char *desc;
    work_ctx ctx = alloc_ctx();
    int64_t len, dlen;
    char error[128];
    int ret = 0;
    int not_ready;

    while (1) {
      int r;
#ifdef __native_client__
      r = wait_for_chan(output_app_label, NULL, NULL, NULL, NULL, NULL);
      if (r) {
        ret = -1;
        break;
      }
      not_ready = get_work_desc(ctx, &desc, &dlen, &data, &len);
      if (not_ready)
        continue;
#else
      r = get_work_desc_single(ctx, &desc, &dlen, &data, &len);
      if (r) {
        ret = -1;
        break;
      }
#endif
      if (data == NULL) {
        ret = -1;
        break;
      }
      put_work_desc(ctx, "{}", 3, data, len);
      free(data);
      free(desc);
    }

    return ret;
}

void Usage(char* prog)
{
    printf("Usage: %s <output_app_label: 1 or 0>\n", prog);
}

int main(int argc, char** argv, char** envp) {
  bool output_app_label = false;
  WorkSpec *spec;
  char* rest = NULL;

  if (argc < 3) {
    Usage(argv[0]);
    exit(1);
  }
  output_app_label = (strtol(argv[1], &rest, 0) == 1);
  if (strcmp(rest, "") != 0) {
    Usage(argv[0]);
    exit(1);
  }
  spec = WorkSpec_parse(argv[argc - 1]);

  if (spec) {
    assert(spec->n > 0);
    if (setup_for_work(spec)) {
      fprintf(stderr, "setup failed");
      return 1;
    }
    run_pipeline(output_app_label);
    finish_work(1);
  } else {
    fprintf(stderr, "No spec.. (%d, last arg %s)\n", argc, argv[argc -1]);
    return 1;
  }

}
