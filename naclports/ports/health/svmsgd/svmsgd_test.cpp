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
#include <string>
#include <algorithm>
#include <map>
#include <vector>
#include <cstdlib>
#include <cmath>

#include <fcntl.h>
#include <unistd.h>

#include "assert.h"
#include "vectors.h"
#include "timer.h"
#include "loss.h"
#include "data.h"
#include "svmsgd.h"

using namespace std;

// --- Command line arguments

const char *modelfile = 0;
const char *testfile = 0;
bool normalize = true;
double lambda = 1e-5;
int maxtrain = -1;


void
usage(const char *progname)
{
  const char *s = ::strchr(progname,'/');
  progname = (s) ? s + 1 : progname;
  cerr << "Usage: " << progname << " [options] trainfile modelfile" << endl
       << "Options:" << endl;
#define NAM(n) "    " << setw(16) << left << n << setw(0) << ": "
#define DEF(v) " (default: " << v << ".)"
  cerr << NAM("-lambda x")
       << "Regularization parameter" << DEF(lambda) << endl
       << NAM("-dontnormalize")
       << "Do not normalize the L2 norm of patterns." << endl
       << NAM("-maxtrain n")
       << "Restrict training set to n examples." << endl;
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
          if (testfile == 0)
            testfile = arg;
          else if (modelfile == 0)
            modelfile = arg;
          else
            usage(argv[0]);
        }
      else
        {
          while (arg[0] == '-') 
            arg += 1;
          string opt = arg;
          if (opt == "lambda" && i+1<argc)
            {
              lambda = atof(argv[++i]);
              assert(lambda>0 && lambda<1e4);
            }
          else if (opt == "dontnormalize")
            {
              normalize = false;
            }
          else if (opt == "maxtrain" && i+1 < argc)
            {
              maxtrain = atoi(argv[++i]);
              assert(maxtrain > 0);
            }
          else
            {
              cerr << "Option " << argv[i] << " not recognized." << endl;
              usage(argv[0]);
            }

        }
    }
  if (! testfile || ! modelfile)
    usage(argv[0]);
}

void 
config(const char *progname)
{
  cout << "# Running: " << progname;
  cout << " -lambda " << lambda;
  if (! normalize) cout << " -dontnormalize";
  if (maxtrain > 0) cout << " -maxtrain " << maxtrain;
  cout << endl;
#define NAME(x) #x
#define NAME2(x) NAME(x)
  cout << "# Compiled with: "
       << " -DLOSS=" << NAME2(LOSS)
       << " -DBIAS=" << BIAS
       << " -DREGULARIZED_BIAS=" << REGULARIZED_BIAS
       << endl;
}

// --- main function

int dims;
xvec_t xtest;
yvec_t ytest;

int main(int argc, const char **argv)
{
  parse(argc, argv);
  config(argv[0]);
  if (testfile)
    load_datafile(testfile, xtest, ytest, dims, normalize);
  cout << "# Number of features " << dims << "." << endl;
  // prepare svm
  int imin = 0;
  int imax = xtest.size() - 1;
  SvmSgd svm(dims, lambda);

  int modelfd = open(modelfile, O_RDONLY);
  svm.load(modelfd);
  close(modelfd);

  Timer timer;
  // test
  svm.test(imin, imax, xtest, ytest, "test:  ");
  return 0;
}
