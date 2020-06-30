# Copyright 2015 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

CTEST_EXECUTABLES="
  basicstuff
  cholesky
  determinant
  geo_transformations
  inverse
"

EXECUTABLES="
  test/basicstuff
  test/cholesky
  test/determinant
  test/geo_transformations
  test/inverse
"

EXTRA_CMAKE_ARGS="-DEIGEN_BUILD_PKGCONFIG=OFF -DEIGEN_SPLIT_LARGE_TESTS=OFF"
MAKE_TARGETS="$CTEST_EXECUTABLES"

# Eigen tests are flakey on the bots:
# https://bugs.chromium.org/p/naclports/issues/detail?id=223
# TODO(sbc): re-enable if we can de-flake the tests
TESTS_DISABLED=1

BuildHost() {
  HOST_BUILD_DIR=${WORK_DIR}/build_host
  HOST_INSTALL_DIR=${NACL_PACKAGES_BUILD}/install_host
  Banner "Build host version"
  MakeDir ${HOST_BUILD_DIR}
  ChangeDir ${HOST_BUILD_DIR}
  LogExecute cmake ${SRC_DIR} ${EXTRA_CMAKE_ARGS} -DCMAKE_INSTALL_PREFIX=${HOST_INSTALL_DIR}
  LogExecute make -j${OS_JOBS} ${MAKE_TARGETS:-}
  LogExecute make install
  cd -
}

BuildStep() {
  (BuildHost)
  DefaultBuildStep
}

#TestStep() {
#  # Eigen has ~600 tests, we only build a few
#  ChangeDir ${BUILD_DIR}
#  if [ ${NACL_ARCH} = "pnacl" ]; then
#    return
#  fi
#  for exe in ${CTEST_EXECUTABLES}; do
#    LogExecute test/$exe.sh
#  done
#}
