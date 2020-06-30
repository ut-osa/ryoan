#!/bin/bash

cd $(dirname $0)

logfile_number=
encrypt=1
padding=1
checkpoint=1

usage="Usage: $0 [{--log-append|-l} string] [--no-checkpoint] [--no-encrypt] [--no-io-model] path/to/pipe_description.json"
while [[ $# -gt 0 ]]
do
key="$1"

case $key in
    -l|--log-append)
    logfile_number="$2"
    shift
    shift
    ;;
    --no-encrypt)
    encrypt=0
    shift # past argument
    ;;
    --no-checkpoint)
    checkpoint=0
    shift # past argument
    ;;
    --help)
    echo "$usage"
    exit 1
    ;;
    --no-io-model)
    padding=0
    shift # past argument
    ;;
    *)    # unknown option
    break
    ;;
esac
done

if [ $# -ne 1 ]; then
   echo "$usage"
   exit 1
fi

export json_file=$1
export port=$[$$ + 6000]
export start_prog=./start_pipeline
export logfile_number

if [[ "$encrypt" == "0" ]]; then
   export RYOAN_NO_ENCRYPT=yes
fi

if [[ "$checkpoint" == "0" ]]; then
   export RYOAN_NO_CHECKPOINT=yes
fi

export time="$(which time)"

# launch the server and wait for initialization
# there are 3 arguments:
#     * the command which starts the server
#     * the number of ryoan instances
#     * the name of the log file
launch_server() {
   cmd=$1
   N=$2
   logfile=$3
   i=0
   exec 3< <($cmd 2>&1 || true)
   while [  $i -lt $N ]; do
      sed '/-----------CHECKPOINT---------$/q' <&3
      let i+=1
   done
   cat <&3 >$logfile &
}
