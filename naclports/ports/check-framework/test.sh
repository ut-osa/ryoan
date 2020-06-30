#!/bin/bash
CUR_DIR=$(pwd)
BUILD_DIR="${CUR_DIR}/../../out/build/check-framework/build_x86-64_$1"
cd $BUILD_DIR
if [ "$1" = "glibc" ]; then
    make installcheck prefix=${NACL_SDK_ROOT}/toolchain/linux_x86_glibc/x86_64-nacl/usr/
elif [ "$1" = "clang-newlib" ]; then
    make installcheck prefix=${NACL_SDK_ROOT}/toolchain/linux_pnacl/x86_64-nacl/usr/
fi
cd $CUR_DIR
