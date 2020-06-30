# Copyright 2015 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

if [ "${NACL_LIBC}" = "newlib" ]; then
    NACLPORTS_CPPFLAGS+=" -I${NACLPORTS_INCLUDE}/glibc-compat"
fi

NACLPORTS_LIBS="-lz -llzma"

BuildHost() {
  HOST_BUILD_DIR=${WORK_DIR}/build_host
  HOST_INSTALL_DIR=${NACL_PACKAGES_BUILD}/install_host
  Banner "Build host version"
  MakeDir ${HOST_BUILD_DIR}
  ChangeDir ${HOST_BUILD_DIR}
  LogExecute cmake -DCMAKE_INSTALL_PREFIX=${HOST_INSTALL_DIR} \
    -DCMAKE_INCLUDE_PATH=${HOST_INSTALL_DIR}/include \
    -DCMAKE_LIBRARY_PATH=${HOST_INSTALL_DIR}/lib \
    -DCMAKE_EXE_LINKER_FLAGS="-L${HOST_INSTALL_DIR}/lib -Wl,-rpath=${HOST_INSTALL_DIR}/lib -llzma" \
    -DCMAKE_CXX_FLAGS=" -I${HOST_INSTALL_DIR}/include" \
    -DCMAKE_C_FLAGS=" -I${HOST_INSTALL_DIR}/include" \
    -DSTATIC_LIBRARY_FLAGS="-L${HOST_INSTALL_DIR}/lib" -DPTHREAD_LIB=pthread \
    -DDL_LIB=dl ${SRC_DIR}
  LogExecute make -j${OS_JOBS} VERBOSE=1
  LogExecute make install
  cd -
}

ConfigureStep() {
    LogExecute rm -rf ${SRC_DIR} ${BUILD_DIR}
    LogExecute MakeDir ${SRC_DIR}
    LogExecute MakeDir ${BUILD_DIR}
    LogExecute cp -rf ${START_DIR}/* ${SRC_DIR}
    (BuildHost)
    cd ${BUILD_DIR}
    EXTRA_CMAKE_ARGS="-DEXEEXT:STRING=.nexe -DNACL_SYS_LIB=nacl_sys_private -DPTHREAD_LIB=pthread_private -DGLIBC-COMPAT=glibc-compat"
    DefaultConfigureStep
}
