# Copyright 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

EXECUTABLES="pngtest${NACL_EXEEXT} pngvalid${NACL_EXEEXT} pngfix${NACL_EXEEXT} \
png-fix-itxt${NACL_EXEEXT} pngimage${NACL_EXEEXT} pngstest${NACL_EXEEXT} \
pngunknown${NACL_EXEEXT}"


BuildHost() {
  HOST_INSTALL_DIR=${NACL_PACKAGES_BUILD}/install_host
  HOST_BUILD_DIR=${WORK_DIR}/build_host
  Banner "Build host version"
  MakeDir ${HOST_BUILD_DIR}
  ChangeDir ${HOST_BUILD_DIR}
  LDFLAGS="-L${HOST_INSTALL_DIR}/lib -Wl,-rpath=${HOST_INSTALL_DIR}/lib" CC="gcc"\
    LogExecute ${SRC_DIR}/configure --prefix=${HOST_INSTALL_DIR} \
    --with-zlib-prefix=${HOST_INSTALL_DIR}/lib
  LogExecute make -j${OS_JOBS}
  LogExecute make install
}

ConfigureStep() {
  (BuildHost)
  ChangeDir ${BUILD_DIR}
  DefaultConfigureStep
}
