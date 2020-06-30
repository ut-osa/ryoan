// -*- C++ -*-
// SVM with stochastic gradient
// Copyright (C) 2007- Leon Bottou

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA

#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>

#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>

#include "svmsgd.h"
#include "request.h"

#include "pipeline/pipeline.h"
#include "pipeline/worker.h"

//using namespace std;

#define BUFSIZE (4096 << 2)
unsigned char buffer[BUFSIZE];

const char *prog_name = "svmsgd_model_loader_pipeline";

unsigned char n_models;
SvmSgd *models;

#define SvmSgdLoadItem(fd, x) if (read(fd, &x, sizeof(x)) < (ssize_t)sizeof(x)) return false;
// before becoming tainted
bool load_models(int modelsfd) {
  SvmSgdLoadItem(modelsfd, n_models);
  models = new SvmSgd[n_models];
  for (int i = 0; i < n_models; i++) {
    models[i].load(modelsfd);
  }
  close(modelsfd);
  return true;
}

void unload_models() {
  delete[] models;
}

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
    r = wait_for_chan(true, ctx, (uint8_t **)&desc, (uint64_t *)&dlen, (uint8_t **)&data, (uint64_t *)&len);
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
    info = (health_request_info *)data;
    tmp = (unsigned char *)models[info->disease_id - 1].save((char *)buffer, BUFSIZE);
    len = tmp - buffer + info->data_len;
    if (len > BUFSIZE) {
      ret = -1;
      fprintf(stderr, "buffer too small\n");
      break;
    }
    memcpy(tmp, info->data, info->data_len);
    put_work_desc(ctx, desc, dlen, buffer, len);
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

  if (argc < 3) { // the last one is added by pipeline framework
    fprintf(stderr, "Usage: %s <models file>\n", argv[0]);
    return 1;
  }

  int modelsfd = open(argv[1], O_RDONLY);
  if (modelsfd < 0) {
    fprintf(stderr, "error opening models file\n");
    return 1;
  }
  if (!load_models(modelsfd)) {
    fprintf(stderr, "error loading models\n");
    return 1;
  };
  ret = run_pipeline();
  finish_work(1);
  unload_models();
  return ret;
}
