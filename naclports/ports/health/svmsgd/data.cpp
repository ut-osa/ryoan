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


#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <map>
#include <string>
#include <vector>

#define WITH_LIBZ 0

#if WITH_LIBZ
#include "gzstream.h"
#endif

#include "assert.h"
#include "data.h"

using namespace std;




// Compat

int load_datafile(const char *filename, 
                  xvec_t &xp, yvec_t &yp, int &maxd,
                  bool norm, int maxn)
{
  Loader loader(filename);
  int maxdim = 0;
  int pcount, ncount;
  loader.load(xp, yp, norm, maxn, &maxdim, &pcount, &ncount);
  if (pcount + ncount > 0)
    cout << "# Read " << pcount << "+" << ncount 
         << "=" << pcount+ncount << " examples " 
         << "from \"" << filename << "\"." << endl;
  if (maxd < maxdim)
    maxd = maxdim;
  return pcount + ncount;
}


int load_databuffer(const char *buf, 
                  xvec_t &xp, yvec_t &yp, int &maxd,
                  bool norm, int maxn)
{
  Loader loader(NULL, buf);
  int maxdim = 0;
  int pcount, ncount;
  loader.load(xp, yp, norm, maxn, &maxdim, &pcount, &ncount);
  if (maxd < maxdim)
    maxd = maxdim;
  return pcount + ncount;
}


// Loader

struct Loader::Private
{
  string filename;
#if WITH_LIBZ
  bool compressed;
#endif
  bool binary;
  bool fromBuffer;
#if WITH_LIBZ
  igzstream gs;
#endif
  ifstream fs;
  istringstream ss;
};

Loader::~Loader()
{
  delete p;
}

Loader::Loader(const char *name, const char *buffer)
  : p(new Private)
{
  p->filename = name;
#if WITH_LIBZ
  p->compressed = p->binary = false;
#endif
  p->fromBuffer = false;
  if (buffer) {
    p->fromBuffer = true;
    p->ss.str(buffer);
#if WITH_LIBZ
  } else if (p->compressed) {
    p->gs.open(name);
#endif
  } else
    p->fs.open(name);
#if WITH_LIBZ
  if (! (p->eompressed ? p->gs.good() : p->fs.good()))
#else
  if (!p->fs.good())
#endif
    assertfail("Cannot open " << p->filename);
}


int Loader::load(xvec_t &xp, yvec_t &yp, bool normalize, int maxrows,
                 int *p_maxdim, int *p_pcount, int *p_ncount)
{
#if WITH_LIBZ
  istream &f = (p->fromBuffer) ? (istream&)(p->ss) : (
      (p->compressed) ? (istream&)(p->gs) : (istream&)(p->fs));
#else
  istream &f = (p->fromBuffer) ? (istream&)(p->ss) :
      (istream&)(p->fs);
#endif
  bool binary = p->binary;
  int ncount = 0;
  int pcount = 0;
  while (f.good() && maxrows--)
    {
      SVector x;
      double y;
      if (binary)
        {
          y = (f.get()) ? +1 : -1;
          x.load(f);
        }
      else
        {
          f >> std::skipws >> y >> std::ws;
          if (f.peek() == '|') f.get();
          f >> x;
        }
      if (f.good())
        {
          if (normalize)
            {
              double d = dot(x,x);
              if (d > 0 && d != 1.0)
                x.scale(1.0 / sqrt(d)); 
            }
          if (y != +1 && y != -1)
            assertfail("Label should be +1 or -1.");
          xp.push_back(x);
          yp.push_back(y);
          if (y > 0)
            pcount += 1;
          else
            ncount += 1;
          if (p_maxdim && x.size() > *p_maxdim)
            *p_maxdim = x.size();
        }
    }
  if (!f.eof() && !f.good())
    assertfail("Cannot read data from " << p->filename);
  if (p_pcount)
    *p_pcount = pcount;
  if (p_ncount)
    *p_ncount = ncount;
  return pcount + ncount;
}




