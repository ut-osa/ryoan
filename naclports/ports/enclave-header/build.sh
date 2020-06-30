# Copyright 2015 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
ConfigureStep() {
    return
}

BuildStep() {
    return
}

InstallStep() {
    HOST_INSTALL_DIR=${NACL_PACKAGES_BUILD}/install_host
    INCDIR=${DESTDIR_INCLUDE}
    Remove ${INCDIR}
    MakeDir ${INCDIR}
    ChangeDir ${START_DIR}
    for file in *.h ; do 
        LogExecute install -m 644 ${file} ${INCDIR}/
        LogExecute install -m 644 ${file} ${HOST_INSTALL_DIR}/include
    done
}
