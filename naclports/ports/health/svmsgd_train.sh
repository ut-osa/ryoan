#!/bin/bash

exec "${NACL_SDK_ROOT}/tools/sel_ldr_x86_64" -a -q -p "${NACL_SDK_ROOT}/toolchain/linux_pnacl/x86_64-nacl/usr/bin/svmsgd_train.nexe" "$@"
