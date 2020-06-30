#!/bin/bash

if [ $# -ne 3 ]; then
  echo usage: "$0 <disease id, integer> <data point id, integer> port"
  exit -1
fi

data=`head -n$2 ../data/samples/d$1.tst |tail -n1 | cut -d' ' -f2-`
echo == data is ==
echo $data
echo
echo == result is ==
echo $data |./classify_client localhost $3
echo
