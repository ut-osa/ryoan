#! /bin/bash
file="input/test11.dat"
user="testuser11"

./dspam-app -x -u $user -i $file
./dspam-app -y -u $user -i $file

res=`./dspam-app -c -u $user -i $file`

if [[ "$res" == *"Dspam-Classification: innocent"* ]]; then
    exit 0
fi
exit 1