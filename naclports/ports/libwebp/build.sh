# Copyright 2014 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

if [[ ${NACL_SHARED} != 1 ]]; then
  # Without this the test for libpng fails with undefined math functions
  NACLPORTS_LIBS+=" -lz -lm"
fi

EXECUTABLES="
  examples/dwebp${NACL_EXEEXT}
  examples/cwebp${NACL_EXEEXT}
"

BuildHost() {
  HOST_INSTALL_DIR=${NACL_PACKAGES_BUILD}/install_host
  HOST_BUILD_DIR=${WORK_DIR}/build_host
  Banner "Build host version"
  MakeDir ${HOST_BUILD_DIR}
  ChangeDir ${HOST_BUILD_DIR}
  LDFLAGS="-L${HOST_INSTALL_DIR}/lib -Wl,-rpath=${HOST_INSTALL_DIR}/lib" \
    CC="gcc" LogExecute ${SRC_DIR}/configure --prefix=${HOST_INSTALL_DIR} \
    --with-jpegincludedir=${HOST_INSTALL_DIR}/include \
    --with-jpeglibdir=${HOST_INSTALL_DIR}/lib \
    --with-pngincludedir=${HOST_INSTALL_DIR}/include \
    --with-pnglibdir=${HOST_INSTALL_DIR}/lib \
    --with-tiffincludedir=${HOST_INSTALL_DIR}/include \
    --with-tifflibdir=${HOST_INSTALL_DIR}/lib
  LogExecute make -j${OS_JOBS}
  LogExecute make install
}

ConfigureStep() {
  (BuildHost)
  ChangeDir ${BUILD_DIR}
  DefaultConfigureStep
}

#TestStep() {
#  if [[ ${TOOLCHAIN} == pnacl ]]; then
#    return
#  fi
#
#  EXAMPLE_DIR=examples
#  if [[ ${NACL_SHARED} == 1 ]]; then
#    EXAMPLE_DIR=examples/.libs
#    export SEL_LDR_LIB_PATH=${BUILD_DIR}/src/.libs
#  fi
#  LogExecute ${EXAMPLE_DIR}/dwebp ${SRC_DIR}/examples/test.webp -o out.webp
#  LogExecute ${EXAMPLE_DIR}/cwebp out.webp -o out.png
#}
