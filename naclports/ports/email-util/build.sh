BuildHost() {
  HOST_BUILD_DIR=${WORK_DIR}/build_host
  HOST_INSTALL_DIR=${NACL_PACKAGES_BUILD}/install_host
  LogExecute MakeDir ${HOST_INSTALL_DIR}/include/email-pipeline/
  LogExecute install -m 644 ${START_DIR}/util.h ${HOST_INSTALL_DIR}/include/email-pipeline/util.h
  cd -
}

ConfigureStep() {
   (BuildHost)
   return 0
}

BuildStep() {
   return 0
}

InstallStep() {
    LogExecute MakeDir ${DESTDIR_INCLUDE}/email-util/
    LogExecute install -m 644 ${START_DIR}/util.h ${DESTDIR_INCLUDE}/email-util/util.h
}
