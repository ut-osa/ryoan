#! /bin/bash
fileA="input/test13a.dat"
fileB="input/test13b.dat"
user="testuser13"

./dspam-app -y -u $user -i $fileA

res=`./dspam-app -c -u $user -i $fileB`

if [[ "$res" == *"Dspam-Classification: innocent"* ]]; then
    exit 0
fi
exit 1