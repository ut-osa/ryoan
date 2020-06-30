CXX ?=g++
OPT=
OPTS=-g -O2
CXXFLAGS= ${OPTS} ${OPT} -Wall -Werror -Wextra -fno-threadsafe-statics \
		  -fvisibility-inlines-hidden -std=gnu++11 \
		  -fno-rtti -fno-exceptions -fno-strict-aliasing

CXXFLAGS += ${CPPFLAGS} ${NACLPORTS_CXXFLAGS}
LIBS ?= -lpthread
EXEEXT ?= ""
PROGRAMS = classify_server${EXEEXT} classify_client${EXEEXT}
OBJS =
INCS =

all: ${PROGRAMS}

clean:
	-rm ${PROGRAMS} 2>/dev/null
	-rm *.o 2>/dev/null

classify_server.o: classify_server.cpp ${INCS}
	${CXX} ${CXXFLAGS} -c -o $@ classify_server.cpp

classify_client.o: classify_client.cpp ${INCS}
	${CXX} ${CXXFLAGS} -c -o $@ classify_client.cpp

classify_server${EXEEXT}: classify_server.o ${OBJS}
	${CXX} ${CXXFLAGS} -o $@ classify_server.o ${OBJS} ${LDFLAGS} ${LIBS}

classify_client${EXEEXT}: classify_client.o ${OBJS}
	${CXX} ${CXXFLAGS} -o $@ classify_client.o ${OBJS} ${LDFLAGS} ${LIBS}
