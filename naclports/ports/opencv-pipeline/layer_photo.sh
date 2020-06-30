#!/bin/bash

NACL_SDK_ROOT=/home/zhitingz/Documents/native_client_sources/nacl_sdk/pepper_canary

${NACL_SDK_ROOT}/tools/sel_ldr_x86_64 -q -a -B ${NACL_SDK_ROOT}/tools/irt_core_x86_64.nexe -p ${NACL_SDK_ROOT}/toolchain/linux_pnacl/x86_64-nacl/usr/bin/layer_photo.nexe
