#! /bin/bash
# similar to test 14, but using -p instead of -c
withSubject="input/test1.dat"
noSubject="input/test14.dat"
user="testuser15"

./dspam-app -x -u $user -i $withSubject

res=`./dspam-app -p -u $user -i $noSubject`

if [[ "$res" == *"Dspam-Classification: spam"* ]]; then
    exit 0
fi
exit 1
