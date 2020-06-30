# Copyright (c) 2011 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

BUILD_DIR=${SRC_DIR}
EXECUTABLES="bzip2"

EnableCliMain
NACLPORTS_CFLAGS+=" -fPIC"

ConfigureStep() {
  return
}

BuildHost() {
  HOST_INSTALL_DIR=${NACL_PACKAGES_BUILD}/install_host
  MakeDir ${HOST_INSTALL_DIR}
  Banner "Build host x86_64 version"
  LogExecute make clean || true
  LogExecute make -j${OS_JOBS} libbz2.a bzip2
  LogExecute make -f Makefile-libbz2_so clean || true
  LogExecute make -f Makefile-libbz2_so -j${OS_JOBS}
  MakeDir ${HOST_INSTALL_DIR}/bin
  MakeDir ${HOST_INSTALL_DIR}/include
  MakeDir ${HOST_INSTALL_DIR}/lib
  LogExecute cp -f bzlib.h ${HOST_INSTALL_DIR}/include
  LogExecute cp -f bzip2 ${HOST_INSTALL_DIR}/bin
  LogExecute chmod a+r ${HOST_INSTALL_DIR}/include/bzlib.h
  LogExecute cp -f libbz2.a ${HOST_INSTALL_DIR}/lib
  if [ -f libbz2.so.1.0  ]; then
      if [ -f libbz2.so ]; then
        LogExecute rm libbz2.so
      fi
      LogExecute ln -s libbz2.so.1.0 libbz2.so
      LogExecute cp -af libbz2.so* ${HOST_INSTALL_DIR}/lib
  fi
  LogExecute chmod a+r ${HOST_INSTALL_DIR}/lib/libbz2.*
  LogExecute make clean || true
  LogExecute make -f Makefile-libbz2_so clean || true
}

BuildStep() {
  (BuildHost)
  SetupCrossEnvironment
  LogExecute make clean
  LogExecute make \
      CC="${CC}" AR="${AR}" RANLIB="${RANLIB}" \
      CFLAGS="${CFLAGS}" LDFLAGS="${LDFLAGS}" \
      -j${OS_JOBS} libbz2.a bzip2
  if [ "${NACL_SHARED}" = "1" ]; then
    LogExecute make -f Makefile-libbz2_so clean
    LogExecute make -f Makefile-libbz2_so \
      CC="${CC}" AR="${AR}" RANLIB="${RANLIB}" \
      CFLAGS="${CFLAGS}" LDFLAGS="${LDFLAGS}" \
      -j${OS_JOBS}
  fi
}

InstallStep() {
  # Don't rely on make install, as it implicitly builds executables
  # that need things not available in newlib.
  MakeDir ${DESTDIR_INCLUDE}
  MakeDir ${DESTDIR_LIB}
  MakeDir ${DESTDIR}${PREFIX}/bin
  LogExecute cp -f bzlib.h ${DESTDIR_INCLUDE}
  LogExecute cp -f bzip2 ${DESTDIR}${PREFIX}/bin
  LogExecute chmod a+r ${DESTDIR_INCLUDE}/bzlib.h

  LogExecute cp -f libbz2.a ${DESTDIR_LIB}
  if [ -f libbz2.so.1.0 ]; then
    if [ -f libbz2.so ]; then
      LogExecute rm libbz2.so
    fi
    LogExecute ln -s libbz2.so.1.0 libbz2.so
    LogExecute cp -af libbz2.so* ${DESTDIR_LIB}
  fi
  LogExecute chmod a+r ${DESTDIR_LIB}/libbz2.*
}
