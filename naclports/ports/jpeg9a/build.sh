# Copyright (c) 2011 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

EXECUTABLES="rdjpgcom${NACL_EXEEXT} wrjpgcom${NACL_EXEEXT} \
      cjpeg${NACL_EXEEXT} djpeg${NACL_EXEEXT} jpegtran${NACL_EXEEXT}"

BuildHost() {
  HOST_INSTALL_DIR=${NACL_PACKAGES_BUILD}/install_host
  HOST_BUILD_DIR=${WORK_DIR}/build_host
  Banner "Build host version"
  MakeDir ${HOST_BUILD_DIR}
  ChangeDir ${HOST_BUILD_DIR}
  LDFLAGS="-L${HOST_INSTALL_DIR}/lib -Wl,-rpath=${HOST_INSTALL_DIR}/lib" CC="gcc"\
    LogExecute ${SRC_DIR}/configure --prefix=${HOST_INSTALL_DIR}
  LogExecute make -j${OS_JOBS}
  LogExecute make install
}

ConfigureStep() {
  (BuildHost)
  ChangeDir ${BUILD_DIR}
  DefaultConfigureStep
}

#TestStep() {
#  export SEL_LDR_LIB_PATH=$PWD/.libs
#  if [ "${NACL_ARCH}" = "pnacl" ]; then
#    for arch in x86-32 x86-64; do
#      for exe in ${EXECUTABLES}; do
#        local exe_noext=${exe%.*}
#        WriteLauncherScriptPNaCl ${exe_noext} ${exe_noext}.${arch}.nexe ${arch}
#      done
#      make test
#    done
#  else
#    if [ "${NACL_SHARED}" = "1" ]; then
#      for exe in ${EXECUTABLES}; do
#       local script=$(basename ${exe%.*})
#        WriteLauncherScript ${script} ${exe}
#      done
#    fi
#    make test
#  fi
#}
