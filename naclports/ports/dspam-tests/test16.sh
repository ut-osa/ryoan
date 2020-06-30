#! /bin/bash
# see if we read past html tags in emails
fileA="input/test16a.dat"
fileB="input/test16b.dat"
user="testuser16"

./dspam-app -x -u $user -i $fileA

res=`./dspam-app -p -u $user -i $fileB`

if [[ "$res" == *"Dspam-Classification: spam"* ]]; then
    exit 0
fi
exit 1
