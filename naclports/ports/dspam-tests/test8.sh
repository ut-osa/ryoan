#! /bin/bash
data=`cat input/test8.dat`
user="testuser8"
home="./etc"

./dspam-app -y -u $user -h $home "$data"

res=`./dspam-app -c -u $user -h $home "$data an additional string of words not originally in the input file"`

if [[ "$res" == *"Dspam-Classification: innocent"* ]]; then
    exit 0
fi
exit 1