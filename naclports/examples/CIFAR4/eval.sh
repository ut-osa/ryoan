#!/bin/bash
COMPILE_DIR_PREFIX="~/.theano/compiledir_Linux-4.8.17MLFS-x86_64-with-Ubuntu-16.04-xenial-x86_64-3.5.2-64"
export THEANO_FLAGS="compiledir=$COMPILE_DIR_PREFIX-base/"

cd $(dirname $0)

export argline="cifar_eval.py $@"
exec ../ryoan_run.py -b

