#!/bin/bash

if [ $# -ne 1 ]; then
  echo usage: "$0 port"
  exit -1
fi

./classify_server $1 ../svmsgd/svmsgd_classify ../data/samples/all.models
