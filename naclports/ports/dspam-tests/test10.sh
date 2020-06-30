#! /bin/bash
file="input/test10.dat"
user="testuser10"

res=`./dspam-app -c -u $user -i $file`

if [[ "$res" == *"Dspam-Classification: innocent"* ]]; then
    exit 0
fi
exit 1