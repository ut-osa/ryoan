#! /bin/bash
data=`cat input/test9.dat`
user="testuser9"

res=`./dspam-app -c -u $user "$data"`

if [[ "$res" == *"Dspam-Classification: innocent"* ]]; then
    exit 0
fi
exit 1