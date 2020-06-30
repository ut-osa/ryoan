#!/bin/bash
COMPILE_DIR_PREFIX="~/.theano/compiledir_Linux-4.8.17MLFS-x86_64-with-Ubuntu-16.04-xenial-x86_64-3.5.2-64-clang"
MAIN_SCRIPT="cifar_ps.py"
PORT=26379
N=8
cd $(dirname $0)
rm dump.*
kill $(ps aux | grep 'redis-server \*:'$PORT | awk '{print $2}')
sleep 1

../ryoan_run.py -l -b -p $PORT & 
THEANO_FLAGS="compiledir=$COMPILE_DIR_PREFIX-0/" argline="$MAIN_SCRIPT 0 $N" nohup ../ryoan_run.py -b > ./logs/cifar_trainer$N.log & 
THEANO_FLAGS="compiledir=$COMPILE_DIR_PREFIX-1/" argline="$MAIN_SCRIPT 1 $N" nohup ../ryoan_run.py -b > ./logs/cifar_trainer$N.log &
THEANO_FLAGS="compiledir=$COMPILE_DIR_PREFIX-2/" argline="$MAIN_SCRIPT 2 $N" nohup ../ryoan_run.py -b > ./logs/cifar_trainer$N.log &
THEANO_FLAGS="compiledir=$COMPILE_DIR_PREFIX-3/" argline="$MAIN_SCRIPT 3 $N" nohup ../ryoan_run.py -b > ./logs/cifar_trainer$N.log &
THEANO_FLAGS="compiledir=$COMPILE_DIR_PREFIX-4/" argline="$MAIN_SCRIPT 4 $N" nohup ../ryoan_run.py -b > ./logs/cifar_trainer$N.log &
THEANO_FLAGS="compiledir=$COMPILE_DIR_PREFIX-5/" argline="$MAIN_SCRIPT 5 $N" nohup ../ryoan_run.py -b > ./logs/cifar_trainer$N.log &
THEANO_FLAGS="compiledir=$COMPILE_DIR_PREFIX-6/" argline="$MAIN_SCRIPT 6 $N" nohup ../ryoan_run.py -b > ./logs/cifar_trainer$N.log &
THEANO_FLAGS="compiledir=$COMPILE_DIR_PREFIX-7/" argline="$MAIN_SCRIPT 7 $N" nohup ../ryoan_run.py -b > ./logs/cifar_trainer$N.log &

