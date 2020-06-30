# Copyright (c) 2013 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

export ac_cv_func_gethostbyname=yes
export ac_cv_func_getaddrinfo=no
export ac_cv_func_connect=yes


EnableGlibcCompat

EXECUTABLES="src/curl${NACL_EXEEXT}"

EXTRA_CONFIGURE_ARGS+=" --without-libssh2"
if [ "${NACL_DEBUG}" = "1" ]; then
  NACLPORTS_CPPFLAGS+=" -DDEBUGBUILD"
  EXTRA_CONFIGURE_ARGS+=" --enable-debug --disable-curldebug"
fi

BuildHost() {
  HOST_INSTALL_DIR=${NACL_PACKAGES_BUILD}/install_host
  HOST_BUILD_DIR=${WORK_DIR}/build_host
  Banner "Build host version"
  MakeDir ${HOST_BUILD_DIR}
  ChangeDir ${HOST_BUILD_DIR}
  LDFLAGS="-L${HOST_INSTALL_DIR}/lib -Wl,-rpath=${HOST_INSTALL_DIR}/lib" \
    CC="gcc" LogExecute ${SRC_DIR}/configure --prefix=${HOST_INSTALL_DIR} \
    ${EXTRA_CONFIGURE_ARGS:-}
  LogExecute make -j${OS_JOBS}
  LogExecute make install
  MakeDir ${HOST_INSTALL_DIR}/share/curl
  LogExecute ${SRC_DIR}/lib/mk-ca-bundle.pl \
    ${HOST_INSTALL_DIR}/share/curl/ca-bundle.crt
}

ConfigureStep() {
  (BuildHost)
  # Can't use LIBS here otherwise nacl-spawn gets linked into the libcurl.so
  # which we don't ever want.
  export EXTRA_LIBS="${NACL_CLI_MAIN_LIB}"
  NACLPORTS_LDFLAGS+=" ${NACL_CLI_MAIN_LDFLAGS}"
  ChangeDir ${BUILD_DIR}
  DefaultConfigureStep
}

InstallStep() {
  DefaultInstallStep
  MakeDir ${DESTDIR}${PREFIX}/share/curl
  LogExecute ${SRC_DIR}/lib/mk-ca-bundle.pl \
      ${DESTDIR}${PREFIX}/share/curl/ca-bundle.crt
}

PublishStep() {
  if [ "${NACL_SHARED}" = "1" ]; then
    EXECUTABLE_DIR=.libs
  else
    EXECUTABLE_DIR=.
  fi

  MakeDir ${PUBLISH_DIR}

  local exe=${PUBLISH_DIR}/curl_${NACL_ARCH}${NACL_EXEEXT}

  LogExecute cp ${DESTDIR}${PREFIX}/share/curl/ca-bundle.crt ${PUBLISH_DIR}
  LogExecute cp src/${EXECUTABLE_DIR}/curl${NACL_EXEEXT} ${exe}
  if [ "${NACL_ARCH}" = "pnacl" ]; then
    LogExecute ${PNACLFINALIZE} ${exe}
  fi

  pushd ${PUBLISH_DIR}
  LogExecute python ${NACL_SDK_ROOT}/tools/create_nmf.py \
      ${PUBLISH_DIR}/curl*${NACL_EXEEXT} \
      -L${DESTDIR_LIB} \
      -s . \
      -o curl.nmf
  popd

  InstallNaClTerm ${PUBLISH_DIR}
  LogExecute cp ${START_DIR}/index.html ${PUBLISH_DIR}
  LogExecute cp ${START_DIR}/curl.js ${PUBLISH_DIR}
}
