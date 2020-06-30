# Copyright 2015 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

EXECUTABLES="tests/check_check_export${NACL_EXEEXT} tests/check_check${NACL_EXEEXT} \
tests/check_mem_leaks${NACL_EXEEXT} tests/check_nofork${NACL_EXEEXT} \
tests/check_nofork_teardown${NACL_EXEEXT} tests/check_stress${NACL_EXEEXT} \
tests/check_thread_stress${NACL_EXEEXT} tests/ex_output${NACL_EXEEXT}"

EXTRA_CONFIGURE_ARGS+=" --enable-fork=no --disable-silent-rules"
if [ "${NACL_SHARED}" = "1" ]; then
    EXTRA_CONFIGURE_ARGS+=" --enable-shared=yes"
else
    EXTRA_CONFIGURE_ARGS+=" --enable-shared=no --enable-static=yes"
fi
EnableGlibcCompat

ConfigureStep() {
  ChangeDir ${SRC_DIR}
  autoreconf --install
  PatchConfigure
  ChangeDir ${BUILD_DIR}
  DefaultConfigureStep
}
