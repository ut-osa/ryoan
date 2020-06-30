# Copyright 2015 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

EXTRA_CONFIGURE_ARGS+=" --with-zlib=${NACL_PREFIX} \
--with-libbz2-prefix=${NACL_PREFIX} --with-libcheck-prefix=${NACL_PREFIX} \
--with-xml=${NACL_PREFIX} --with-openssl=${NACL_PREFIX} \
--with-libncurses-prefix=${NACL_PREFIX} --with-libcurl=${NACL_PREFIX} \
--libdir=${PREFIX}/lib \
--enable-llvm=no --enable-milter=no --enable-debug=yes --disable-silent-rules \
--prefix=${NACL_PREFIX}"

EXECUTABLES="clamscan/clamscan${NACL_EXEEXT} clamconf/clamconf${NACL_EXEEXT} \
sigtool/sigtool${NACL_EXEEXT} freshclam/freshclam${NACL_EXEEXT} \
clambc/clambc${NACL_EXEEXT}"
#clamd/clamd${NACL_EXEEXT}
# clamdscan/clamdscan${NACL_EXEEXT} 

TEST_EXECUTABLES="clamscan/.libs/test_clamscan${NACL_EXEEXT} \
clamconf/.libs/test_clamconf${NACL_EXEEXT} \
sigtool/.libs/test_sigtool${NACL_EXEEXT} \
freshclam/.libs/test_freshclam${NACL_EXEEXT} \
clambc/.libs/test_clambc${NACL_EXEEXT}"
#clamd/.libs/test_clamd${NACL_EXEEXT}
# clamdscan/.libs/test_clamdscan${NACL_EXEEXT} \

UNITTEST_EXECUTABLES="unit_tests/test_check_clamd${NACL_EXEEXT}"

BuildHost() {
  HOST_INSTALL_DIR=${NACL_PACKAGES_BUILD}/install_host
  HOST_BUILD_DIR=${WORK_DIR}/build_host
  Banner "Build host version"
  MakeDir ${HOST_BUILD_DIR}
  ChangeDir ${HOST_BUILD_DIR}
  FLAG="-g -O3"
  LDFLAGS="-L${HOST_INSTALL_DIR}/lib -Wl,-rpath=${HOST_INSTALL_DIR}/lib" \
      CC="gcc $FLAG" LIBS=" -lz -lpipeline -lcrypto -lm" CPPFLAGS="-D__OSA__" \
      CFLAGS=$FLAG LogExecute ${SRC_DIR}/configure \
    --prefix=${HOST_INSTALL_DIR} \
    --with-zlib=${HOST_INSTALL_DIR} \
    --with-libbz2-prefix=${HOST_INSTALL_DIR} \
    --with-libcheck-prefix=${HOST_INSTALL_DIR} \
    --with-xml=${HOST_INSTALL_DIR} \
    --with-openssl=${HOST_INSTALL_DIR} \
    --with-libncurses-prefix=${HOST_INSTALL_DIR} \
    --with-libcurl=${HOST_INSTALL_DIR} \
    --libdir=${HOST_INSTALL_DIR}/lib \
    --enable-llvm=no --enable-milter=no --enable-debug=yes\
    --disable-silent-rules --disable-pthreads \
    --enable-static=yes --enable-shared=no --enable-ltld-install=no \
	--disable-unrar --enable-check=no
  LogExecute make -j${OS_JOBS}
  LogExecute make install
}

ConfigureStep() {
  (BuildHost)
  if [ "${NACL_SHARED}" = "1" ]; then
    EXTRA_CONFIGURE_ARGS+=" --enable-shared=yes --enable-static=yes"
    EXECUTABLES+=" clamdtop/clamdtop${NACL_EXEEXT} \
      clamsubmit/.libs/clamsubmit${NACL_EXEEXT}"
    TEST_EXECUTABLES+=" clamdtop/.libs/test_clamdtop${NACL_EXEEXT} \
      clamsubmit/.libs/test_clamsubmit${NACL_EXEEXT}"
    UNITTEST_EXECUTABLES+=" unit_tests/.libs/test_check_clamav${NACL_EXEEXT} \
      unit_tests/.libs/test_check_fpu_endian${NACL_EXEEXT}"
  else
    EXTRA_CONFIGURE_ARGS+=" --enable-static=yes --enable-shared=no \
      --enable-ltdl-install=no --disable-unrar --disable-pthreads --enable-check=no"
    UNITTEST_EXECUTABLES+=" unit_tests/test_check_clamav${NACL_EXEEXT} \
      unit_tests/test_check_fpu_endian${NACL_EXEEXT}"
    NACLPORTS_CPPFLAGS+=" -I${NACLPORTS_INCLUDE}/glibc-compat -fPIC -std=c99"
    NACLPORTS_CFLAGS+=" -Wall -Wextra"
    export LIBS+=" -lz -lpipeline -lcrypto -lm -lglibc-compat -lnacl_sys_private"
  fi
  ChangeDir ${BUILD_DIR}
  DefaultConfigureStep
}

#NACLPORTS_LDFLAGS+=" -pthread"

#
# Write a wrapper script that will run a nexe under sel_ldr
# $1 - Script name
# $2 - Nexe name
#
WriteTestLauncherScript() {
  local script=$1
  local binary=$2
  echo $script
  echo $binary
  if [ "${SKIP_SEL_LDR_TESTS}" = "1" ]; then
    return
  fi

  if [ "${OS_NAME}" = "Cygwin" ]; then
    local LOGFILE=nul
  else
    local LOGFILE=/dev/null
  fi

  if [ "${NACL_LIBC}" = "glibc" ]; then
    cat > "$script" <<HERE
#!/bin/bash
SCRIPT_DIR=\$(dirname "\${BASH_SOURCE[0]}")
if [ \$(uname -s) = CYGWIN* ]; then
  SCRIPT_DIR=\$(cygpath -m \${SCRIPT_DIR})
fi
NACL_SDK_ROOT=${NACL_SDK_ROOT}

NACL_SDK_LIB=${NACL_SDK_LIB}
LIB_PATH_DEFAULT=${NACL_SDK_LIBDIR}:${NACLPORTS_LIBDIR}:\
${INSTALL_DIR}/${PREFIX}/lib
LIB_PATH_DEFAULT=\${LIB_PATH_DEFAULT}:\${NACL_SDK_LIB}:\${SCRIPT_DIR}
SEL_LDR_LIB_PATH=\${SEL_LDR_LIB_PATH}:\${LIB_PATH_DEFAULT}

"\${NACL_SDK_ROOT}/tools/sel_ldr.py" -p --library-path "\${SEL_LDR_LIB_PATH}" \
    -- "\${SCRIPT_DIR}/$binary" "\$@"
HERE
  else
    cat > "$script" <<HERE
#!/bin/bash
SCRIPT_DIR=\$(dirname "\${BASH_SOURCE[0]}")
if [ \$(uname -s) = CYGWIN* ]; then
  SCRIPT_DIR=\$(cygpath -m \${SCRIPT_DIR})
fi
NACL_SDK_ROOT=${NACL_SDK_ROOT}

"\${NACL_SDK_ROOT}/tools/sel_ldr.py" -p -- "\${SCRIPT_DIR}/$binary" "\$@"
HERE
  fi

  chmod 750 "$script"
  echo "Wrote script $script -> $binary"
}

PostBuildStep() {
  DefaultPostBuildStep
  local prefix=text_
  if [ "${NACL_SHARED}" = "1" ]; then
      for nexe in ${TEST_EXECUTABLES}; do
          # Create a script which will run the executable in sel_ldr.  The name
          # of the script is the same as the name of the executable, either without
          # any extension or with the .sh extension.
          base_binary=$(basename ${nexe})
      
          if [[ ${nexe} == *${NACL_EXEEXT} && ! -d ${nexe%%${NACL_EXEEXT}} ]]; then
              LogExecute WriteTestLauncherScript "${nexe%%${NACL_EXEEXT}}" \
                         "${base_binary#test_}"
          else
              LogExecute WriteTestLauncherScript "${nexe}.sh" "${base_binary#test_}"
          fi
      done
  fi
}

InstallStep() {
  DefaultInstallStep
  LogExecute cp -r ${DESTDIR}/${NACL_PREFIX}/bin ${DESTDIR}/${PREFIX}/bin
  LogExecute cp -r ${DESTDIR}/${NACL_PREFIX}/etc ${DESTDIR}/${PREFIX}/etc
  LogExecute cp -r ${DESTDIR}/${NACL_PREFIX}/include ${DESTDIR}/${PREFIX}/include
  LogExecute cp -r ${DESTDIR}/${NACL_PREFIX}/sbin ${DESTDIR}/${PREFIX}/sbin
  LogExecute cp -r ${DESTDIR}/${NACL_PREFIX}/share ${DESTDIR}/${PREFIX}/share
  LogExecute rm -rf ${DESTDIR}/home
}

PostInstallTestStep() {
  echo "commented out"
#  cd unit_tests
#  rm -rf check_clamav.nexe check_clamd.nexe check_fpu_endian.nexe
#  make check_clamav.nexe
#  make check_clamd.nexe
#  make check_fpu_endian.nexe
#  cd ..
#  local prefix=text_
#  for  nexe in ${UNITTEST_EXECUTABLES}; do
#      # Create a script which will run the executable in sel_ldr.  The name
#      # of the script is the same as the name of the executable, either without
#      # any extension or with the .sh extension.
#      base_binary=$(basename ${nexe})
#      
#      if [[ ${nexe} == *${NACL_EXEEXT} && ! -d ${nexe%%${NACL_EXEEXT}} ]]; then
#          LogExecute WriteTestLauncherScript "${nexe%%${NACL_EXEEXT}}" \
#          "${base_binary#test_}"
#      else
#          LogExecute WriteTestLauncherScript "${nexe}.sh" "${base_binary#test_}"
#      fi
#  done
#  make check
}
