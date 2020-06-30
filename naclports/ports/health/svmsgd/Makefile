# SVM with stochastic gradient

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA


CXX ?=g++
OPT=
OPTS=-g -O2
CXXFLAGS= ${OPTS} ${OPT} -Wall -Werror -Wextra -fno-threadsafe-statics \
		  -fvisibility-inlines-hidden -std=gnu++11 \
		  -fno-rtti -fno-exceptions -fno-strict-aliasing

CXXFLAGS += ${CPPFLAGS} ${NACLPORTS_CXXFLAGS}
LIBS ?= -lz -lm
EXEEXT ?= ""
PROGRAMS = svmsgd_train${EXEEXT} svmsgd_test${EXEEXT} svmsgd_classify${EXEEXT}
OBJS = vectors.o gzstream.o timer.o
INCS = vectors.h timer.h wrapper.h gzstream.h assert.h

all: ${PROGRAMS}

clean:
	-rm ${PROGRAMS} 2>/dev/null
	-rm *.o 2>/dev/null

gzstream.o: gzstream.cpp ${INCS}
	${CXX} ${CXXFLAGS} -c -o $@ gzstream.cpp

vectors.o: vectors.cpp ${INCS}
	${CXX} ${CXXFLAGS} -c -o $@ vectors.cpp

timer.o: timer.cpp ${INCS}
	${CXX} ${CXXFLAGS} -c -o $@ timer.cpp

data.o: data.cpp ${INCS}
	${CXX} ${CXXFLAGS} -c -o $@ data.cpp

svmsgd.o: svmsgd.cpp svmsgd.h data.h loss.h ${INCS}
	${CXX} ${CXXFLAGS} -c -o $@ svmsgd.cpp

svmsgd_train${EXEEXT}: svmsgd_train.o svmsgd.o data.o svmsgd.h ${OBJS}
	${CXX} ${CXXFLAGS} -o $@ svmsgd_train.o svmsgd.o data.o ${OBJS} ${LDFLAGS} ${LIBS}

svmsgd_test${EXEEXT}: svmsgd_test.o svmsgd.o data.o svmsgd.h ${OBJS}
	${CXX} ${CXXFLAGS} -o $@ svmsgd_test.o svmsgd.o data.o ${OBJS} ${LDFLAGS} ${LIBS}

svmsgd_classify${EXEEXT}: svmsgd_classify.o svmsgd.o data.o svmsgd.h ${OBJS}
	${CXX} ${CXXFLAGS} -o $@ svmsgd_classify.o svmsgd.o data.o ${OBJS} ${LDFLAGS} ${LIBS}
