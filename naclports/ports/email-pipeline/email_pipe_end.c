#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <errno.h>

#ifdef __native_client__
#include <request.h>
#else
#include <stdint.h>
#endif

#include "pipeline/pipeline.h"
#include "pipeline/worker.h"
#include "pipeline/json-builder.h"
#include "pipeline/json.h"
#include "util.h"

const char *prog_name = "email_pipe_end";

int run_pipeline(bool output_app_label)
{
    unsigned char *data;
    char *desc;
    work_ctx ctx = alloc_ctx();
    int64_t len, dlen;
    int ret = 0;
    struct pipeline_result res_data;
    uint8_t* buffer;
    uint64_t size, start, end;


    while (1) {
      memset(&res_data, 0, sizeof(struct pipeline_result));
#ifdef __native_client__
      int r = wait_for_chan(output_app_label, NULL, NULL, NULL, NULL, NULL);
      if (r) {
        ret = -1;
        break;
      }
#endif
      while (1) {
        int r = get_work_desc(ctx, &desc, &dlen, &data, &len);
        if (r == 1)
          continue;
        if (r == -1) {
          /*printf(" dspam ------ %s\n", res_data.dinfo.result_str);*/
          /*printf(" desc sent: %s\n", desc);*/
          /*fflush(stdout);*/
          put_work_desc(ctx, desc, dlen, (unsigned char *)&res_data, sizeof(struct pipeline_result));
#ifndef __native_client__
          free(desc);
          free(data);
#endif
          break;
        }
        if (desc == NULL || data == NULL) {
          ret = -1;
          break;
        }
        /*printf(" ------ desc is %s\n", desc);*/
        /*fflush(stdout);*/
        if (data[0] == APP_ID_DSPAM)
          res_data.dinfo = *(struct d_info *)(data + 1);
        else if (data[0] == APP_ID_CLAMAV)
          res_data.cinfo = *(struct c_info *)(data + 1);
        else {
            ret = -1;
            free(data);
            free(desc);
            break;
        }
#ifdef __native_client__
        free(data);
        free(desc);
#endif
      }
    }
    return ret;
}

int main(int argc, char** argv, char** envp) {
  WorkSpec *spec = WorkSpec_parse(argv[argc - 1]);

  if (spec) {
    assert(spec->n > 0);
    if (setup_for_work(spec)) {
      fprintf(stderr, "setup failed");
      return 1;
    }
    run_pipeline(false);
    finish_work(1);
  } else {
    fprintf(stderr, "No spec..(%d, last arg: %s)\n", argc, argv[argc -1]);
    return 1;
  }
}
