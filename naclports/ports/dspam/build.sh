# Copyright 2015 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

EXECUTABLES="src/dspam${NACL_EXEEXT} src/dspamc${NACL_EXEEXT} \
src/tools/dspam_stats${NACL_EXEEXT} src/tools/dspam_2sql${NACL_EXEEXT} \
src/tools/dspam_admin${NACL_EXEEXT} src/tools/dspam_clean${NACL_EXEEXT} \
src/tools/dspam_crc${NACL_EXEEXT} src/tools/dspam_dump${NACL_EXEEXT} \
src/tools/dspam_merge${NACL_EXEEXT}"

EXTRA_CONFIGURE_ARGS+=" --enable-syslog=no \
--with-storage-driver=sqlite3_drv --with-sqlite-includes=${NACLPORTS_INCLUDE} \
--with-sqlite-libraries=${NACLPORTS_LIBDIR} --enable-shared=no \
--enable-static=yes --enable-dlopen=no"

#EXTRA_CONFIGURE_ARGS+=" --enable-warnings --enable-debug --enable-verbose-debug \
#--with-logfile=${NACL_PREFIX}/var/dspam/log/dspam.log --disable-syslog \
#--with-storage-driver=sqlite3_drv --with-sqlite-includes=${NACLPORTS_INCLUDE} \
#--with-sqlite-libraries=${NACLPORTS_LIBDIR} --enable-shared=no \
#--enable-static=yes --enable-dlopen=no"

BuildHost() {
  HOST_INSTALL_DIR=${NACL_PACKAGES_BUILD}/install_host
  HOST_BUILD_DIR=${WORK_DIR}/build_host
  Banner "Build host version"
  MakeDir ${HOST_BUILD_DIR}
  ChangeDir ${HOST_BUILD_DIR}
  LDFLAGS="-L${HOST_INSTALL_DIR}/lib -Wl,-rpath=${HOST_INSTALL_DIR}/lib" \
    LogExecute ${SRC_DIR}/configure --prefix=${HOST_INSTALL_DIR} --enable-syslog=no \
    --with-storage-driver=sqlite3_drv \
    --with-sqlite-includes=${HOST_INSTALL_DIR}/include \
    --with-sqlite-libraries="${HOST_INSTALL_DIR}/lib"
  LogExecute make clean || true
  LogExecute make -j${OS_JOBS}
  LogExecute make install
}

ConfigureStep() {
  (BuildHost)
  ChangeDir ${BUILD_DIR}
  NACLPORTS_CFLAGS="${NACLPORTS_CFLAGS/-DNDEBUG/}"
  export ac_cv_search_dlopen="none required"
  DefaultConfigureStep
}

EnableGlibcCompat

InstallStep() {
  DefaultInstallStep
  extra_path=${NACL_PREFIX#/}
  extra_path=${extra_path%%/*}
  LogExecute rm -rf ${INSTALL_DIR}/${extra_path}
  LogExecute MakeDir ${NACL_PREFIX}/var/dspam/log
}
