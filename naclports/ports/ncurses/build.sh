# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

EXECUTABLES="progs/clear${NACL_EXEEXT}"

EXTRA_CONFIGURE_ARGS+=" --disable-database"
EXTRA_CONFIGURE_ARGS+=" --with-fallbacks=xterm-256color,vt100"
EXTRA_CONFIGURE_ARGS+=" --disable-termcap"
# Without this ncurses headers will be installed include/ncurses
EXTRA_CONFIGURE_ARGS+=" --enable-overwrite"
EXTRA_CONFIGURE_ARGS+=" --without-ada"

if [ "${NACL_SHARED}" = 1 ]; then
  EXTRA_CONFIGURE_ARGS+=" --with-shared"
fi

if [ "${TOOLCHAIN}" = "pnacl" ]; then
  EXTRA_CONFIGURE_ARGS+=" --without-cxx-binding"
fi


EnableGlibcCompat

export cf_cv_ar_flags=${NACL_ARFLAGS}
NACL_ARFLAGS=""

if [[ ${TOOLCHAIN} == emscripten ]]; then
  export cf_cv_posix_c_source=no
fi

if [[ ${TOOLCHAIN} == glibc ]]; then
  export ac_cv_func_sigvec=no
fi

BuildHost() {
  HOST_INSTALL_DIR=${NACL_PACKAGES_BUILD}/install_host
  HOST_BUILD_DIR=${WORK_DIR}/build_host
  Banner "Build host version"
  export ac_cv_func_sigvec=no
  MakeDir ${HOST_BUILD_DIR}
  ChangeDir ${HOST_BUILD_DIR}
  LDFLAGS="-L${HOST_INSTALL_DIR}/lib -Wl,-rpath=${HOST_INSTALL_DIR}/lib" \
    CC="gcc-4.8" LogExecute ${SRC_DIR}/configure --prefix=${HOST_INSTALL_DIR} \
    ${EXTRA_CONFIGURE_ARGS:-} --with-shared
  LogExecute make -j${OS_JOBS}
  LogExecute make install
  ChangeDir ${HOST_INSTALL_DIR}/lib
  LogExecute ln -sf libncurses.a libtermcap.a
}

ConfigureStep() {
  BuildHost
  ChangeDir ${BUILD_DIR}
  DefaultConfigureStep
}

InstallStep() {
  DefaultInstallStep
  ChangeDir ${DESTDIR_LIB}
  LogExecute ln -sf libncurses.a libtermcap.a
}
