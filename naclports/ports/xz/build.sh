# Copyright 2015 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

export EXTRA_LIBS="${NACL_CLI_MAIN_LIB}"

NACLPORTS_LIBS+="-pthread -lnacl_io -l${NACL_CXX_LIB}"

EnableGlibcCompat

BuildX32Host() {
  HOST_INSTALL_DIR=${NACL_PACKAGES_BUILD}/install_host
  HOST_BUILD_DIR=${WORK_DIR}/build_x32_host
  Banner "Build host x32 version"
  ChangeDir ${BUILD_DIR}
  LDFLAGS="-mx32 -L${HOST_INSTALL_DIR}/libx32 -Wl,-rpath=${HOST_INSTALL_DIR}/libx32" \
         CFLAGS="-mx32" CC="gcc" LogExecute ${SRC_DIR}/configure \
         --prefix=${HOST_INSTALL_DIR} --libdir="${HOST_INSTALL_DIR}/libx32"
  LogExecute make -j${OS_JOBS}
  LogExecute make install
}

ConfigureStep() {
  (BuildX32Host)
  DefaultConfigureStep
}
