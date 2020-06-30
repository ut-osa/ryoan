# Copyright 2015 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

BUILD_DIR=${SRC_DIR}

EXECUTABLES="test0${NACL_EXEEXT} test1${NACL_EXEEXT} test2${NACL_EXEEXT} \
test3${NACL_EXEEXT} test4${NACL_EXEEXT} test5${NACL_EXEEXT} \
test6${NACL_EXEEXT} dspam-app${NACL_EXEEXT}"

BuildHost() {
  export EXEEXT=""
	rm -f dspam-app
  rm -f dspam-app.nexe
	rm -f test
  rm -rf ./etc/log
	for i in {0..6}; do
		rm -rf test$i test$i.nexe
	done
  HOST_INSTALL_DIR=${NACL_PACKAGES_BUILD}/install_host
  export LDFLAGS=" -L${HOST_INSTALL_DIR}/lib -Wl,-rpath=${HOST_INSTALL_DIR}/lib -lpipeline -lcrypto"
  export CFLAGS=" -I${HOST_INSTALL_DIR}/include"
  ./configure DSPAM_INSTALL="${HOST_INSTALL_DIR}"
  make && make build-test
  # make run-all-tests
  LogExecute install -m 744  dspam-app ${HOST_INSTALL_DIR}/bin/dspam-app
}

ConfigureStep() {
  LogExecute cp -rf ${START_DIR}/* .
  (BuildHost)
	rm -f dspam-app
  rm -f dspam-app.nexe
	rm -f test
	rm -rf ./etc/data
  rm -rf ./etc/log
	for i in {0..6}; do
		rm -rf test$i test$i.nexe
	done
  ./configure DSPAM_INSTALL="${NACL_PREFIX}"
}

BuildStep() {
  # export the nacl tools
  NACLPORTS_CFLAGS+=" -I${NACLPORTS_INCLUDE}/glibc-compat"
  NACLPORTS_LDFLAGS+=" -lpipeline -lcrypto -lglibc-compat -lnacl_sys_private"
  export CC=${NACLCC}
  export CXX=${NACLCXX}
  export AR=${NACLAR}
  export NACL_SDK_VERSION
  export NACL_SDK_ROOT
  export LDFLAGS=${NACLPORTS_LDFLAGS}
  export CPPFLAGS=${NACLPORTS_CPPFLAGS}
  export CFLAGS=${NACLPORTS_CFLAGS}
  export PTHREAD_LIB="-lpthread_private"
  export EXEEXT=".nexe"
  DefaultBuildStep
  make build-test
}

PostInstallTestStep() {
  # make run-all-tests
  echo "comment out"
}

InstallStep() {
  MakeDir ${DESTDIR_BIN}
  LogExecute install -m 744 dspam-app.nexe ${DESTDIR_BIN}/dspam-app.nexe
}
