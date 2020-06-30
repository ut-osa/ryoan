#! /bin/bash
# see if test16 works the same if you classify the non-html version first
fileA="input/test16a.dat"
fileB="input/test16b.dat"
user="testuser17"

./dspam-app -x -u $user -i $fileB

res=`./dspam-app -p -u $user -i $fileA`

if [[ "$res" == *"Dspam-Classification: spam"* ]]; then
    exit 0
fi
exit 1
