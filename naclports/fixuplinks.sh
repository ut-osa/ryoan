#!/bin/bash
set -eu

cd $(dirname $0)

nacl_root=../native_client
nacl_build=$nacl_root/scons-out/nacl-x86-64
special_includes="${nacl_root}/src/untrusted/nacl/ryoan_ops.h ${nacl_root}/src/untrusted/nacl/request.h"

if [ ! -d $nacl_build ]
then
	echo "build nacl first"
	exit 1
fi

echo "Fixing up symlinks"
for lib in $(find $nacl_build/lib -mindepth 1 -exec readlink -f \{\} \;)
do
	ln -sf $lib nacl_sdk/pepper_canary/toolchain/linux_pnacl/x86_64-nacl/lib/$(basename $lib)
	ln -sf $lib nacl_sdk/pepper_canary/lib/clang-newlib_x86_64/Debug/$(basename $lib)
	ln -sf $lib nacl_sdk/pepper_canary/lib/clang-newlib_x86_64/Release/$(basename $lib)
done

for incl in $(find $nacl_build/include -mindepth 1 -exec readlink -f \{\} \;) $special_includes
do
   ln -sf $(readlink -f $incl) nacl_sdk/pepper_canary/toolchain/linux_pnacl/x86_64-nacl/include/$(basename $incl)
done
