#! /bin/bash
# test spam detection with stripping of subject
withSubject="input/test1.dat"
noSubject="input/test14.dat"
user="testuser14"

./dspam-app -x -u $user -i $withSubject

res=`./dspam-app -c -u $user -i $noSubject`

if [[ "$res" == *"Dspam-Classification: spam"* ]]; then
    exit 0
fi
exit 1
