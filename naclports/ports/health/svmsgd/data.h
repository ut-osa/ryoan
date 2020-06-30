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

#ifndef DATA_H
#define DATA_H

#include <vector>
#include "vectors.h"

typedef std::vector<SVector> xvec_t;
typedef std::vector<double>  yvec_t;

class Loader
{
public:
  ~Loader();
  Loader(const char *filename, const char *buf=NULL);
  int load(xvec_t &xp, yvec_t &yp, bool norm=true, int maxn=-1,
           int *maxdim=0, int *pcount=0, int *ncount=0);
private:
  struct Private;
  Private *p;
};


int load_datafile(const char *filename, 
                  xvec_t &xp, yvec_t &yp, int &maxd,
                  bool norm=true, int maxn=-1);

int load_databuffer(const char *buf,
                  xvec_t &xp, yvec_t &yp, int &maxd,
                  bool norm=true, int maxn=-1);
#endif
