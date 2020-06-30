# Copyright (c) 2011 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

MAKE_TARGETS="libxml2.la"
INSTALL_TARGETS="install-libLTLIBRARIES install-data install-binSCRIPTS"
EXTRA_CONFIGURE_ARGS="--with-python=no"
EXTRA_CONFIGURE_ARGS+=" --with-iconv=no"

BuildHost() {
  HOST_INSTALL_DIR=${NACL_PACKAGES_BUILD}/install_host
  HOST_BUILD_DIR=${WORK_DIR}/build_host
  Banner "Build host version"
  MakeDir ${HOST_BUILD_DIR}
  ChangeDir ${HOST_BUILD_DIR}
  LDFLAGS="-L${HOST_INSTALL_DIR}/lib -Wl,-rpath=${HOST_INSTALL_DIR}/lib" \
    CC="gcc" LogExecute ${SRC_DIR}/configure --prefix=${HOST_INSTALL_DIR} \
    ${EXTRA_CONFIGURE_ARGS:-} --with-zlib=${HOST_INSTALL_DIR}/lib
  LogExecute make -j${OS_JOBS} ${MAKE_TARGETS:-}
  LogExecute make ${INSTALL_TARGETS:-install}
}

ConfigureStep() {
  (BuildHost)
  ChangeDir ${BUILD_DIR}
  DefaultConfigureStep
}
