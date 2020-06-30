#!/bin/bash

set -eu

HERE=$(readlink -f $(dirname $0))
BUILD="./build-libsodium"
SRC="${HERE}/libsodium-1.0.10"

mkdir -p ${BUILD}
(cd ${BUILD} && ${SRC}/configure)

## to download new virus databases:
#(cd data/email && rm -rf virus_db && mkdir virus_db && cd virus_db && \
#   wget http://database.clamav.net/main.cvd && \
#   wget http://database.clamav.net/daily.cvd && \
#   wget http://database.clamav.net/bytecode.cvd)
