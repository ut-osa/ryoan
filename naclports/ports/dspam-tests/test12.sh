#! /bin/bash
file="input/test12.dat"
user="testuser12"

./dspam-app -y -u $user -i $file
./dspam-app -x -u $user -i $file

res=`./dspam-app -c -u $user -i $file`

if [[ "$res" == *"Dspam-Classification: spam"* ]]; then
    exit 0
fi
exit 1