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

#include <fcntl.h>
#include <unistd.h>

#include "timer.h"
#include "assert.h"
#include "vectors.h"
#include "data.h"
#include "svmsgd.h"

using namespace std;

// --- Command line arguments

const char *trainfile = 0;
const char *oldmodelfile = 0;
const char *newmodelfile = 0;
bool normalize = true;
double lambda = 1e-5;
int epochs = 1;
int maxtrain = -1;


void
usage(const char *progname)
{
  const char *s = ::strchr(progname,'/');
  progname = (s) ? s + 1 : progname;
  cerr << "Usage: " << progname << " [options] trainfile new_modelfile [old_modelfile]" << endl
       << "Options:" << endl;
#define NAM(n) "    " << setw(16) << left << n << setw(0) << ": "
#define DEF(v) " (default: " << v << ".)"
  cerr << NAM("-lambda x")
       << "Regularization parameter" << DEF(lambda) << endl
       << NAM("-epochs n")
       << "Number of training epochs" << DEF(epochs) << endl
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
          if (trainfile == 0)
            trainfile = arg;
          else if (newmodelfile == 0)
            newmodelfile = arg;
          else if (oldmodelfile == 0)
            oldmodelfile = arg;
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
          else if (opt == "epochs" && i+1<argc)
            {
              epochs = atoi(argv[++i]);
              assert(epochs>0 && epochs<1e6);
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
  if (! trainfile || !newmodelfile)
    usage(argv[0]);
}

void 
config(const char *progname)
{
  cout << "# Running: " << progname;
  cout << " -lambda " << lambda;
  cout << " -epochs " << epochs;
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
xvec_t xtrain;
yvec_t ytrain;

int main(int argc, const char **argv)
{
  parse(argc, argv);
  config(argv[0]);
  if (trainfile)
    load_datafile(trainfile, xtrain, ytrain, dims, normalize, maxtrain);
  cout << "# Number of features " << dims << "." << endl;
  // prepare svm
  int imin = 0;
  int imax = xtrain.size() - 1;
  SvmSgd svm(dims, lambda);


  int oldmodelfd = -1;
  if (oldmodelfile)
    oldmodelfd = open(oldmodelfile, O_RDONLY);
  if (oldmodelfd >= 0) {
    svm.load(oldmodelfd);
    close(oldmodelfd);
  }

  Timer timer;
  // determine eta0 using sample
  int smin = 0;
  int smax = imin + min(1000, imax);
  svm.determineEta0(smin, smax, xtrain, ytrain);
  // train
  for(int i=0; i<epochs; i++) {
    cout << "--------- Epoch " << i+1 << "." << endl;
    timer.start();
    svm.train(imin, imax, xtrain, ytrain);
    timer.stop();
    cout << "Total training time " << setprecision(6) 
      << timer.elapsed() << " secs." << endl;
  }
  int newmodelfd = open(newmodelfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  svm.save(newmodelfd);
  close(newmodelfd);
  return 0;
}
