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
#include <cstdlib>

#include <fcntl.h>
#include <unistd.h>

#include "vectors.h"
#include "svmsgd.h"

#include "pipeline/pipeline.h"
#include "pipeline/worker.h"

using namespace std;

#define BUFSIZE (4096 << 2)
unsigned char buffer[BUFSIZE];

const char *prog_name = "svmsgd_classify_pipeline";

const char *modelfile = 0;
bool normalize = true;

void
usage(const char *progname)
{
  const char *s = ::strchr(progname,'/');
  progname = (s) ? s + 1 : progname;
  cerr << "Usage: " << progname << " [options] " << endl
       << "Options:" << endl;
#define NAM(n) "    " << setw(16) << left << n << setw(0) << ": "
#define DEF(v) " (default: " << v << ".)"
  cerr << NAM("-dontnormalize")
       << "Do not normalize the L2 norm of patterns." << endl;
#undef NAM
#undef DEF
  ::exit(10);
}

void
parse(int argc, const char **argv)
{
  for (int i=1; i<argc; i++)
    {
      const char *arg = argv[i];
      if (arg[0] != '-')
        {
          usage(argv[0]);
        }
      else
        {
          while (arg[0] == '-')
            arg += 1;
          string opt = arg;
          if (opt == "dontnormalize")
            {
              normalize = false;
            }
          else
            {
              cerr << "Option " << argv[i] << " not recognized." << endl;
              usage(argv[0]);
            }

        }
    }
}

int run_pipeline()
{
  int ret = 0;
  unsigned char *data, *tmp;
  char *desc;
  work_ctx ctx = alloc_ctx();
  int64_t len, dlen;
  SvmSgd model;

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
      cerr << "error get_work_desc" << endl;
      break;
    }
#endif
    if (desc == NULL || data == NULL) {
      ret = -1;
      cerr << "error desc or data is NULL" << endl;
      break;
    }
    printf(" ----- %s get desc %s\n", prog_name, desc);
    tmp = (unsigned char *)model.load((char *)data, len);

    istringstream iss((char *)tmp);
    ostringstream oss;
    SVector x;
    if (!iss.good()) {
      ret = -1;
      cerr << "istringstream problem" << endl;
      break;
    }
    iss >> x;
    if (iss.fail()) {
      ret = -1;
      cerr << "error loading input vector" << endl;
      break;
    }

    oss << model.testOne(x, 0, NULL, NULL);
    len = oss.str().length() + 1; // include '\0'
    put_work_desc(ctx, desc, dlen, (unsigned char *)oss.str().c_str(), len);
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

  parse(argc - 1, argv);
  ret = run_pipeline();
  finish_work(1);
  return ret;
}
