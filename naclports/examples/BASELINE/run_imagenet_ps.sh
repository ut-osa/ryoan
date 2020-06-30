#!/bin/bash
COMPILE_DIR_PREFIX="~/.theano/compiledir_Linux-4.8.17MLFS-x86_64-with-Ubuntu-16.04-xenial-x86_64-3.5.2-64-BASELINE"
MAIN_SCRIPT="imagenet_ps.py"

cd $(dirname $0)
rm dump.*
kill $(ps aux | grep 'redis-server \*:6379' | awk '{print $2}')
sleep 1

THEANO_FLAGS="compiledir=$COMPILE_DIR_PREFIX-0/" argline="$MAIN_SCRIPT 0 4" ../ryoan_run.py -l -b > ./logs/imagenet_trainer4.log & 
THEANO_FLAGS="compiledir=$COMPILE_DIR_PREFIX-1/" argline="$MAIN_SCRIPT 1 4" nohup ../ryoan_run.py -b > ./logs/imagenet_trainer4.log &
THEANO_FLAGS="compiledir=$COMPILE_DIR_PREFIX-2/" argline="$MAIN_SCRIPT 2 4" nohup ../ryoan_run.py -b > ./logs/imagenet_trainer4.log &
THEANO_FLAGS="compiledir=$COMPILE_DIR_PREFIX-3/" argline="$MAIN_SCRIPT 3 4" nohup ../ryoan_run.py -b > ./logs/imagenet_trainer4.log &

