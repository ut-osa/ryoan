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

THEANO_FLAGS="compiledir=${COMPILE_DIR_PREFIX}-0/" argline="cifar_example.py 0 4 $MSEC" ../ryoan_run.py -l -m $OPTIONS
