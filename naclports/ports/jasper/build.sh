# Copyright 2015 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

EXECUTABLES="src/appl/imgcmp src/appl/imginfo src/appl/jasper src/appl/tmrdemo"

EXTRA_CONFIGURE_ARGS+=" --with-pic"

BuildHost() {
  HOST_INSTALL_DIR=${NACL_PACKAGES_BUILD}/install_host
  HOST_BUILD_DIR=${WORK_DIR}/build_host
  Banner "Build host version"
  MakeDir ${HOST_BUILD_DIR}
  ChangeDir ${HOST_BUILD_DIR}
  CPPFLAGS="-I${HOST_INSTALL_DIR}/include" \
    LDFLAGS="-L${HOST_INSTALL_DIR}/lib -Wl,-rpath=${HOST_INSTALL_DIR}/lib" \
    LogExecute ${SRC_DIR}/configure --prefix=${HOST_INSTALL_DIR} \
    ${EXTRA_CONFIGURE_ARGS:-}
  LogExecute make -j${OS_JOBS}
  LogExecute make install
}

ConfigureStep() {
  (BuildHost)
  ChangeDir ${BUILD_DIR}
  DefaultConfigureStep
}

BuildStep(){
  export EXEEXT=${NACL_EXEEXT}
  DefaultBuildStep
}
