# Copyright (c) 2011 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

BUILD_DIR=${SRC_DIR}
EnableGlibcCompat

if [ "${TOOLCHAIN}" = "pnacl" -o "${TOOLCHAIN}" = "clang-newlib" ]; then
  # TODO(sbc): Should probably use clang here
  COMPILER=gcc
else
  COMPILER=gcc
fi

if [ "${TOOLCHAIN}" = "pnacl" -o "${TOOLCHAIN}" = "clang-newlib" ]; then
  COMPILER_VERSION=3.6
elif [ "${NACL_ARCH}" = "arm" ]; then
  COMPILER_VERSION=4.8
else
  COMPILER_VERSION=4.4.3
fi

# TODO(eugenis): build dynamic libraries, too
BUILD_ARGS="
  toolset=${COMPILER}
  target-os=unix
  --build-dir=../${NACL_BUILD_SUBDIR}
  --stagedir=../${NACL_BUILD_SUBDIR}
  link=static"

BUILD_ARGS+=" --without-python"
BUILD_ARGS+=" --without-signals"
BUILD_ARGS+=" --without-mpi"
BUILD_ARGS+=" --without-context"
BUILD_ARGS+=" --without-coroutine"

if [ "${NACL_LIBC}" != "glibc" ] ; then
  BUILD_ARGS+=" --without-locale"
  BUILD_ARGS+=" --without-log"
  BUILD_ARGS+=" --without-timer"
fi

if [ "${NACL_ARCH}" = "arm" -o "${NACL_ARCH}" = "pnacl" ]; then
  # The ARM toolchains are not currently built with full C++ threading support
  # (_GLIBCXX_HAS_GTHREADS is not defined)
  # PNaCl currently doesn't support -x c++-header which threading library
  # tries to use during the build.
  BUILD_ARGS+=" --without-math"
  BUILD_ARGS+=" --without-thread"
  BUILD_ARGS+=" --without-wave"
fi

BuildHost() {
  HOST_INSTALL_DIR=${NACL_PACKAGES_BUILD}/install_host
  MakeDir ${HOST_INSTALL_DIR}
  Banner "Build host version"
  conf="using zlib : 1.2.8 : <include>${HOST_INSTALL_DIR}/include <search>${HOST_INSTALL_DIR}/lib ;
using gcc : : g++-4.8 ;"
  echo $conf > ./tools/build/v2/user-config.jam
  rm -rf bin.v2
  ./bootstrap.sh --without-libraries=python --prefix=${HOST_INSTALL_DIR}
  LDFLAGS="-L${HOST_INSTALL_DIR}/lib -Wl,-rpath=${HOST_INSTALL_DIR}/lib" \
         ./b2 -j4 --prefix=${HOST_INSTALL_DIR} --libdir=${HOST_INSTALL_DIR}/lib64 \
         --layout=system link=static address-model=64 architecture=x86 \
         --debug-configuration -d2 install
  rm -rf bin.v2
  cd ${HOST_INSTALL_DIR}/lib64
}

ConfigureStep() {
  (BuildHost)
  flags=
  for flag in ${NACLPORTS_CPPFLAGS}; do
    flags+=" <compileflags>${flag}"
  done
  conf="using ${COMPILER} : ${COMPILER_VERSION} : ${NACLCXX} :${flags} ;"
  echo $conf > tools/build/v2/user-config.jam
  LogExecute ./bootstrap.sh --prefix=${NACL_PREFIX}
}

BuildStep() {
  LogExecute ./b2 -q -a stage -j ${OS_JOBS} ${BUILD_ARGS}
}

InstallStep() {
  LogExecute ./b2 install -d0 --prefix=${DESTDIR}/${PREFIX} ${BUILD_ARGS}
  cd ${DESTDIR}/${PREFIX}/lib
  ln -s libboost_filesystem.a libboost_filesystem-1_55.a
  ln -s libboost_iostreams.a libboost_iostreams-1_55.a
  ln -s libboost_program_options.a libboost_program_options-1_55.a
  ln -s libboost_system.a libboost_system-1_55.a
  ln -s libboost_thread.a libboost_thread-1_55.a
  ln -s libboost_unit_test_framework.a libboost_unit_test_framework-1_55.a
  cd -
}
