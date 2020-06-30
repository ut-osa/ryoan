# Copyright 2016 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

BUILD_DIR=${SRC_DIR}
EnableGlibcCompat

COMPILER=gcc
COMPILER_VERSION=3.6

EXECUTABLES="
bin/1-1-Extraction
bin/biconcor
bin/build_binary
bin/consolidate
bin/consolidate-direct
bin/consolidate-reverse
bin/CreateOnDiskPt
bin/dump_counts
bin/evaluator
bin/extract
bin/extract-ghkm
bin/extract-lex
bin/extract-mixed-syntax
bin/extractor
bin/extract-rules
bin/filter
bin/filter-rule-table
bin/fragment
bin/generateSequences
bin/kbmira
bin/lexical-reordering-score
bin/lmbrgrid
bin/merge-sorted
bin/mert
bin/moses
bin/moses_chart
bin/pcfg-extract
bin/pcfg-score
bin/phrase-lookup
bin/pro
bin/processLexicalTable
bin/prunePhraseTable
bin/query
bin/queryLexicalTable
bin/queryOnDiskPt
bin/relax-parse
bin/score
bin/ScoreFeatureTest
bin/score-stsg
bin/sentence-bleu
bin/statistics
bin/symal
bin/TMining
bin/vwtrainer
"

BUILD_ARGS="
  toolset=${COMPILER}
  target-os=unix
  --static
  --link=static
  --without-libsegfault
  -q --without-tcmalloc -a
  -debug-configuration -d 2
  threading=single --nacl-build
  "

BuildHost() {
  HOST_INSTALL_DIR=${NACL_PACKAGES_BUILD}/install_host
  Banner "Build host version"
  conf="using gcc : 4.8 : : <linkflags>-lstdc++ <linkflags>-Wl,--no-as-needed <linkflags>-ldl ;" 
  echo $conf > ./jam-files/boost-build/user-config.jam
  cp ${START_DIR}/util.h ${SRC_DIR}/moses/pipeline_util.h
  CFLAGS="-I${HOST_INSTALL_DIR}/include" \
          LDFLAGS="-latomic -pthread -L${HOST_INSTALL_DIR}/lib -lbz2 -Wl,-rpath=${HOST_INSTALL_DIR}/lib" \
          ./bjam --with-cxx=g++ --with-boost=${HOST_INSTALL_DIR} \
          --with-cmph=${HOST_INSTALL_DIR} toolset=gcc target-os=unix \
          --without-libsegfault -q -a -j4 --without-tcmalloc \
          --debug-configuration -d2 address-model=64 architecture=x86 threading=single \
          --libdir=${HOST_INSTALL_DIR}/lib --bindir=${HOST_INSTALL_DIR}/bin install
}

ConfigureStep() {
  (BuildHost)
  flags=
  for flag in ${NACLPORTS_CPPFLAGS}; do
      flags+=" <compileflags>${flag}"
  done
  flags+=" <linkflags>-pthread <linkflags>-lpthread_private"
  conf="using ${COMPILER} : ${COMPILER_VERSION} : ${NACLCXX} :${flags} ;"
  echo $conf > ./jam-files/boost-build/user-config.jam
  cp ${START_DIR}/util.h ${SRC_DIR}/moses/pipeline_util.h
}

BuildStep() {
  export CC=${NACLCC}
  export CXX=${NACLCXX}
  export AR=${NACLAR}
  export NACL_SDK_VERSION
  export NACL_SDK_ROOT
  export LDFLAGS=${NACLPORTS_LDFLAGS}
  export CPPFLAGS=${NACLPORTS_CPPFLAGS}
  export CFLAGS=${NACLPORTS_CFLAGS}
  LogExecute ./bjam --with-cxx=${NACLCXX} --with-boost=${NACL_PREFIX} \
      --with-cmph=${NACL_PREFIX} -j${OS_JOBS} ${BUILD_ARGS} install
}

InstallStep() {
    LogExecute MakeDir ${DESTDIR}/${PREFIX}/bin
    LogExecute MakeDir ${DESTDIR}/${PREFIX}/lib
    LogExecute cp -r ${SRC_DIR}/bin/* ${DESTDIR}/${PREFIX}/bin
    LogExecute cp -r ${SRC_DIR}/lib/* ${DESTDIR}/${PREFIX}/lib
}
