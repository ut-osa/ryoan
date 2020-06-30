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

#include "assert.h"
#include "vectors.h"
#include "timer.h"
#include "data.h"
#include "svmsgd.h"

using namespace std;

// --- Command line arguments

const char *modelfile = 0;
bool normalize = true;


void
usage(const char *progname)
{
  const char *s = ::strchr(progname,'/');
  progname = (s) ? s + 1 : progname;
  cerr << "Usage: " << progname << " [options] modelfile" << endl
       << "Input from stdin" << endl
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
          if (modelfile == 0)
            modelfile = arg;
          else
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
  if (! modelfile)
    usage(argv[0]);
}

// --- main function

xvec_t xtest;

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

#define MAX_INPUT_SIZE (4096 << 2)

// become tainted at this point
int classify(int infd, int outfd)
{
  char *buf = new char[MAX_INPUT_SIZE + 1];
  ssize_t len;
  SVector x;
  ssize_t nbytes = read(infd, buf, MAX_INPUT_SIZE);
  if (nbytes < 0 || nbytes > MAX_INPUT_SIZE) {
    delete buf;
    return -1;
  }
  buf[nbytes] = 0;
  istringstream iss(buf);
  ostringstream oss;
  delete buf;
  if (!iss.good()) {
    return -1;
  }
  iss >> x;
  if (!iss.good()) {
    return -1;
  }

  for (int i = 0; i < n_models; i++) {
    if (i) {
      oss << ", ";
    }
    oss << models[i].testOne(x, 0, NULL, NULL);
  }
  len = oss.str().length();
  if (len != write(outfd, oss.str().c_str(), len))
    return -1;

  return 0;
}

int main(int argc, const char **argv) {
  //config(argv[0]);
  parse(argc, argv);
  int modelsfd = open(modelfile, O_RDONLY);
  if (!load_models(modelsfd)) {
    cerr << "error loading models" << endl;
  };
  classify(0, 1);
  unload_models();
}
