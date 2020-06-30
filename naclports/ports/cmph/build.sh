# Copyright 2016 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

BUILD_DIR=${SRC_DIR}
EnableGlibcCompat

BuildHost() {
    HOST_INSTALL_DIR=${NACL_PACKAGES_BUILD}/install_host
    HOST_BUILD_DIR=${WORK_DIR}/build_host
    Banner "Build host version"
    MakeDir ${HOST_BUILD_DIR}
    ChangeDir ${HOST_BUILD_DIR}
    LDFLAGS="-L${HOST_INSTALL_DIR}/lib -Wl,-rpath=${HOST_INSTALL_DIR}/lib" \
           CFLAGS="-I${SRC_DIR}/src -I${SRC_DIR}" \
           LogExecute ${SRC_DIR}/configure --prefix=${HOST_INSTALL_DIR} \
           --libdir=${HOST_INSTALL_DIR}/lib --bindir=${HOST_INSTALL_DIR}/bin
    LogExecute make clean || true
    LogExecute make -j${OS_JOBS}
    LogExecute make install
}

ConfigureStep() {
    (BuildHost)
    DefaultConfigureStep
}
