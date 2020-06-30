#!/bin/bash

set -eu

PACKAGES="gcc-multilib g++-multilib zlib1g-dev zlib1g-dev:i386 libssl-dev:i386 \
   gcc-4.8 g++-4.8 "

if [ $USER != 'root' ]
then
   echo "This script should be run with sudo" >&2
   exit 1
fi

dpkg --add-architecture i386
apt-get update
apt-get install ${PACKAGES} -y
