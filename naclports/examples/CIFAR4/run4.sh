#!/bin/bash
COMPILE_DIR_PREFIX="~/.theano/Chiron-dir"
N=4
OPTIONS=$@

while [[ $# -gt 1 ]]
do
key="$1"
case $key in
    --ps_msec)
    MSEC="$2"
    shift # past argument
    ;;
    *)
    # unknown option
    ;;
esac
shift # past argument or value
done

if [ -z "$MSEC" ]
then
	LOG_PREFIX="logs/trainer$N"
else
	LOG_PREFIX="logs/trainer$N-ms$MSEC"
fi

cd $(dirname $0)
rm dump.*
kill $(ps aux | grep 'redis-server \*:6379' | awk '{print $2}')
sleep 1

THEANO_FLAGS="compiledir=${COMPILE_DIR_PREFIX}-0/" argline="cifar_example.py 0 4 $MSEC" nohup ../ryoan_run.py -l -m $OPTIONS > $LOG_PREFIX-0.log &
THEANO_FLAGS="compiledir=${COMPILE_DIR_PREFIX}-1/" argline="cifar_example.py 1 4 $MSEC" nohup ../ryoan_run.py -m $OPTIONS > $LOG_PREFIX-1.log &
THEANO_FLAGS="compiledir=${COMPILE_DIR_PREFIX}-2/" argline="cifar_example.py 2 4 $MSEC" nohup ../ryoan_run.py -m $OPTIONS > $LOG_PREFIX-2.log &
THEANO_FLAGS="compiledir=${COMPILE_DIR_PREFIX}-3/" argline="cifar_example.py 3 4 $MSEC" nohup ../ryoan_run.py -m $OPTIONS > $LOG_PREFIX-3.log &


