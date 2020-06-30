# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

NACLPORTS_CPPFLAGS+=" -DMAXPATHLEN=512 -DHAVE_STDARG_H"

if [ "${NACL_SHARED}" = "1" ]; then
  NACLPORTS_CFLAGS+=" -fPIC"
fi

if [ "${NACL_DEBUG}" = "1" ]; then
  NACLPORTS_CPPFLAGS+=" -DDEBUG"
fi

export compat_cv_func_snprintf_works=yes

BuildHost() {
  HOST_INSTALL_DIR=${NACL_PACKAGES_BUILD}/install_host
  HOST_BUILD_DIR=${WORK_DIR}/build_host
  Banner "Build host version"
  MakeDir ${HOST_BUILD_DIR}
  ChangeDir ${HOST_BUILD_DIR}
  LDFLAGS="-L${HOST_INSTALL_DIR}/lib -Wl,-rpath=${HOST_INSTALL_DIR}/lib" \
    CC="gcc" LogExecute ${SRC_DIR}/configure --prefix=${HOST_INSTALL_DIR}
  LogExecute make -j${OS_JOBS}
  LogExecute make install
}

ConfigureStep() {
  (BuildHost)
  ChangeDir ${BUILD_DIR}
  DefaultConfigureStep
}
