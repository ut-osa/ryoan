# Copyright 2015 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

BuildHost() {
  HOST_INSTALL_DIR=${NACL_PACKAGES_BUILD}/install_host
  HOST_BUILD_DIR=${WORK_DIR}/build_host
  Banner "Build host version"
  MakeDir ${HOST_BUILD_DIR}
  ChangeDir ${HOST_BUILD_DIR}
  LDFLAGS="-L${HOST_INSTALL_DIR}/lib -Wl,-rpath=${HOST_INSTALL_DIR}/lib" \
    CC="gcc" LogExecute ${START_DIR}/gnulib/configure \
    --prefix=${HOST_INSTALL_DIR}
  LogExecute make -j${OS_JOBS}
  MakeDir ${HOST_INSTALL_DIR}/lib
  MakeDir ${HOST_INSTALL_DIR}/include
  local LIB=libargp.a
  LogExecute install -m 644 gllib/${LIB} ${HOST_INSTALL_DIR}/lib/${LIB}
  LogExecute install -m 644 ${START_DIR}/include/argp.h ${HOST_INSTALL_DIR}/include/argp.h
}

ConfigureStep() {
  (BuildHost)
  ChangeDir ${BUILD_DIR}
  LogExecute MakeDir ${SRC_DIR}
  LogExecute cp -r ${START_DIR}/* ${SRC_DIR}
  export NACL_CONFIGURE_PATH=${SRC_DIR}/gnulib/configure
  ConfigureStep_Autoconf
}

if [ "${NACL_SHARED}" = "0" ]; then
  NACLPORTS_CFLAGS+=" -I${NACLPORTS_INCLUDE}/glibc-compat"
  NACLPORTS_LDFLAGS+=" -lglibc-compat"
fi

InstallStep() {
  local LIB=libargp.a
  MakeDir ${DESTDIR_LIB}
  Remove ${DESTDIR_INCLUDE}
  MakeDir ${DESTDIR_INCLUDE}
  LogExecute install -m 644 gllib/${LIB} ${DESTDIR_LIB}/${LIB}
  LogExecute install -m 644 ${SRC_DIR}/include/argp.h ${DESTDIR_INCLUDE}/argp.h
}
