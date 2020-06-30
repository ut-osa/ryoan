#!/bin/bash
set -eu

HERE=$(readlink -f $(dirname $0))

NPROCS=$(grep -c ^processor /proc/cpuinfo) 

J=${J:-$NPROCS}

progress_bar=pv
which pv > /dev/null || progress_bar=cat

cd $(dirname $0)
env_file=ryoan_env.tar.xz

dist_uri="https://osa-distribution.s3.amazonaws.com/dist-tarballs"

unzip() {
   local archive=$1

   echo Unpacking $archive
   $progress_bar $archive | tar xJ || return $?
}

test -f $env_file || wget "$dist_uri/ryoan_env.tar.xz"
unzip $env_file

mkdir -p root
mkdir -p build-eglibc
echo Building eglibc
(cd build-eglibc && ../eglibc/configure --prefix=${HERE}/root > config_log 2>&1 && \
   make -j${J} >build_log 2>&1 && make install -j${J} > install_log 2>&1)
(cd apps && ./bootstrap.sh)
