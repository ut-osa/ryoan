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
#include <cmath>

#include <fcntl.h>
#include <unistd.h>

#include "svmsgd.h"
#include "assert.h"
#include "vectors.h"
#include "timer.h"
#include "loss.h"
#include "data.h"

using namespace std;


/// Constructor
SvmSgd::SvmSgd(int dim, double lambda, double eta0)
  : lambda(lambda), eta0(eta0), 
    w(dim), wDivisor(1), wBias(0),
    t(0)
{
}

SvmSgd::SvmSgd() {
}

SvmSgd::SvmSgd(int fd)
{
  load(fd);
}

#define SvmSgdSaveItem(fd, x) if (write(fd, &x, sizeof(x)) < (ssize_t)sizeof(x)) return false;
bool
SvmSgd::save(int fd)
{
  SvmSgdSaveItem(fd, lambda);
  SvmSgdSaveItem(fd, eta0);
  SvmSgdSaveItem(fd, wDivisor);
  SvmSgdSaveItem(fd, wBias);
  SvmSgdSaveItem(fd, t);
  return w.save(fd);
}

#define SvmSgdLoadItem(fd, x) if (read(fd, &x, sizeof(x)) < (ssize_t)sizeof(x)) return false;
bool
SvmSgd::load(int fd)
{
  SvmSgdLoadItem(fd, lambda);
  SvmSgdLoadItem(fd, eta0);
  SvmSgdLoadItem(fd, wDivisor);
  SvmSgdLoadItem(fd, wBias);
  SvmSgdLoadItem(fd, t);
  return w.load(fd);
}

#define SvmSgdSaveItemBuf(buf, len, x) { \
  if (len < (ssize_t)sizeof(x)) return NULL; \
  memcpy(buf, &x, sizeof(x)); \
  buf += (ssize_t)sizeof(x); \
  len -= (ssize_t)sizeof(x); \
}
char *
SvmSgd::save(char *buf, int len)
{
  SvmSgdSaveItemBuf(buf, len, lambda);
  SvmSgdSaveItemBuf(buf, len, eta0);
  SvmSgdSaveItemBuf(buf, len, wDivisor);
  SvmSgdSaveItemBuf(buf, len, wBias);
  SvmSgdSaveItemBuf(buf, len, t);
  return w.save(buf, len);
}

#define SvmSgdLoadItemBuf(buf, len, x) { \
  if (len < (ssize_t)sizeof(x)) return NULL; \
  memcpy(&x, buf, sizeof(x)); \
  buf += (ssize_t)sizeof(x); \
  len -= (ssize_t)sizeof(x); \
}
char *
SvmSgd::load(char *buf, int len)
{
  SvmSgdLoadItemBuf(buf, len, lambda);
  SvmSgdLoadItemBuf(buf, len, eta0);
  SvmSgdLoadItemBuf(buf, len, wDivisor);
  SvmSgdLoadItemBuf(buf, len, wBias);
  SvmSgdLoadItemBuf(buf, len, t);
  return w.load(buf, len);
}

/// Renormalize the weights
void
SvmSgd::renorm()
{
  if (wDivisor != 1.0)
    {
      w.scale(1.0 / wDivisor);
      wDivisor = 1.0;
    }
}

/// Compute the norm of the weights
double
SvmSgd::wnorm()
{
  double norm = dot(w,w) / wDivisor / wDivisor;
#if REGULARIZED_BIAS
  norm += wBias * wBias;
#endif
  return norm;
}

/// Compute the output for one example.
double
SvmSgd::testOne(const SVector &x, double y, double *ploss, double *pnerr)
{
  double s = dot(w,x) / wDivisor + wBias;
  if (ploss)
    *ploss += LOSS::loss(s, y);
  if (pnerr)
    *pnerr += (s * y <= 0) ? 1 : 0;
  return s;
}

/// Perform one iteration of the SGD algorithm with specified gains
void
SvmSgd::trainOne(const SVector &x, double y, double eta)
{
  double s = dot(w,x) / wDivisor + wBias;
  // update for regularization term
  wDivisor = wDivisor / (1 - eta * lambda);
  if (wDivisor > 1e5) renorm();
  // update for loss term
  double d = LOSS::dloss(s, y);
  if (d != 0)
    w.add(x, eta * d * wDivisor);
  // same for the bias
#if BIAS
  double etab = eta * 0.01;
#if REGULARIZED_BIAS
  wBias *= (1 - etab * lambda);
#endif
  wBias += etab * d;
#endif
}


/// Perform a training epoch
void 
SvmSgd::train(int imin, int imax, const xvec_t &xp, const yvec_t &yp, const char *prefix)
{
  cout << prefix << "Training on [" << imin << ", " << imax << "]." << endl;
  assert(imin <= imax);
  assert(eta0 > 0);
  for (int i=imin; i<=imax; i++)
    {
      double eta = eta0 / (1 + lambda * eta0 * t);
      trainOne(xp.at(i), yp.at(i), eta);
      t += 1;
    }
  cout << prefix << setprecision(6) << "wNorm=" << wnorm();
#if BIAS
  cout << " wBias=" << wBias;
#endif
  cout << endl;
}

/// Perform a test pass
void 
SvmSgd::test(int imin, int imax, const xvec_t &xp, const yvec_t &yp, const char *prefix)
{
  cout << prefix << "Testing on [" << imin << ", " << imax << "]." << endl;
  assert(imin <= imax);
  double nerr = 0;
  double loss = 0;
  for (int i=imin; i<=imax; i++)
    testOne(xp.at(i), yp.at(i), &loss, &nerr);
  nerr = nerr / (imax - imin + 1);
  loss = loss / (imax - imin + 1);
  double cost = loss + 0.5 * lambda * wnorm();
  cout << prefix 
       << "Loss=" << setprecision(12) << loss
       << " Cost=" << setprecision(12) << cost 
       << " Misclassification=" << setprecision(4) << 100 * nerr << "%." 
       << endl;
}

/// Perform one epoch with fixed eta and return cost

double 
SvmSgd::evaluateEta(int imin, int imax, const xvec_t &xp, const yvec_t &yp, double eta)
{
  SvmSgd clone(*this); // take a copy of the current state
  assert(imin <= imax);
  for (int i=imin; i<=imax; i++)
    clone.trainOne(xp.at(i), yp.at(i), eta);
  double loss = 0;
  double cost = 0;
  for (int i=imin; i<=imax; i++)
    clone.testOne(xp.at(i), yp.at(i), &loss, 0);
  loss = loss / (imax - imin + 1);
  cost = loss + 0.5 * lambda * clone.wnorm();
  // cout << "Trying eta=" << eta << " yields cost " << cost << endl;
  return cost;
}

void 
SvmSgd::determineEta0(int imin, int imax, const xvec_t &xp, const yvec_t &yp)
{
  const double factor = 2.0;
  double loEta = 1;
  double loCost = evaluateEta(imin, imax, xp, yp, loEta);
  double hiEta = loEta * factor;
  double hiCost = evaluateEta(imin, imax, xp, yp, hiEta);
  if (loCost < hiCost)
    while (loCost < hiCost)
      {
        hiEta = loEta;
        hiCost = loCost;
        loEta = hiEta / factor;
        loCost = evaluateEta(imin, imax, xp, yp, loEta);
      }
  else if (hiCost < loCost)
    while (hiCost < loCost)
      {
        loEta = hiEta;
        loCost = hiCost;
        hiEta = loEta * factor;
        hiCost = evaluateEta(imin, imax, xp, yp, hiEta);
      }
  eta0 = loEta;
  cout << "# Using eta0=" << eta0 << endl;
}

