# Copyright 2015 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

EnableGlibcCompat

BuildHost() {
  HOST_BUILD_DIR=${WORK_DIR}/build_host
  HOST_INSTALL_DIR=${NACL_PACKAGES_BUILD}/install_host
  Banner "Build host x86_64 version"
  MakeDir ${HOST_BUILD_DIR}
  ChangeDir ${HOST_BUILD_DIR}
  LogExecute make clean || true
  LogExecute cmake ${SRC_DIR} -DCMAKE_INSTALL_PREFIX=${HOST_INSTALL_DIR} \
    -DCMAKE_C_FLAGS=" -I${HOST_INSTALL_DIR}/include"
  LogExecute make -j${OS_JOBS}
  LogExecute make install
  cd -
}

ConfigureStep() {
    ChangeDir ${BUILD_DIR}
    LogExecute rm -rf ${SRC_DIR} ${BUILD_DIR}
    LogExecute MakeDir ${SRC_DIR}
    LogExecute MakeDir ${BUILD_DIR}
    LogExecute cp -rf ${START_DIR}/* ${SRC_DIR}
    (BuildHost)
    cd ${BUILD_DIR}
    #NACLPORTS_CFLAGS="${NACLPORTS_CFLAGS/-DNDEBUG/}"
    #EXTRA_CMAKE_ARGS+=" -DCMAKE_BUILD_TYPE=Debug"
    ConfigureStep_CMake
}
