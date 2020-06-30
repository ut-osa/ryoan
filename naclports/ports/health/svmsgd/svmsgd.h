#include <vector>
#include "vectors.h"
#include "data.h"

// ---- Loss function

// Compile with -DLOSS=xxxx to define the loss function.
// Loss functions are defined in file loss.h)
#ifndef LOSS
# define LOSS LogLoss
#endif

// ---- Bias term

// Compile with -DBIAS=[1/0] to enable/disable the bias term.
// Compile with -DREGULARIZED_BIAS=1 to enable regularization on the bias term

#ifndef BIAS
# define BIAS 1
#endif
#ifndef REGULARIZED_BIAS
# define REGULARIZED_BIAS 0
#endif

// ---- Plain stochastic gradient descent

class SvmSgd
{
public:
  SvmSgd(int dim, double lambda, double eta0  = 0);
  SvmSgd(int fd);
  SvmSgd();
  void renorm();
  double wnorm();
  double testOne(const SVector &x, double y, double *ploss, double *pnerr);
  void trainOne(const SVector &x, double y, double eta);
public:
  void train(int imin, int imax, const xvec_t &x, const yvec_t &y, const char *prefix = "");
  void test(int imin, int imax, const xvec_t &x, const yvec_t &y, const char *prefix = "");
public:
  double evaluateEta(int imin, int imax, const xvec_t &x, const yvec_t &y, double eta);
  void determineEta0(int imin, int imax, const xvec_t &x, const yvec_t &y);
  bool save(int fd);
  bool load(int fd);
  char *save(char *buf, int len);
  char *load(char *buf, int len);
private:
  double  lambda;
  double  eta0;
  FVector w;
  double  wDivisor;
  double  wBias;
  double  t;
};
