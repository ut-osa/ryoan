#!/bin/bash
if [ "$NACL_SDK_ROOT" == "" ]; then
  echo "NACL_SDK_ROOT not set"
  exit -1
fi

if [ $# -ne 1 ]; then
  echo usage: "$0 <name of original input file>"
  exit -1
fi

cat $1 | ${NACL_SDK_ROOT}/tools/sel_ldr_x86_64 -q -a -p ${NACL_SDK_ROOT}/toolchain/linux_pnacl/x86_64-nacl/usr/bin/clamscan.nexe --database=virus_db -
