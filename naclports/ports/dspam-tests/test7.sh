#! /bin/bash
data=`cat input/test7.dat`
user="testuser7"
home="./etc"

./dspam-app -x -u $user -h $home "$data AAA"
./dspam-app -x -u $user -h $home "$data BBB"
./dspam-app -x -u $user -h $home "$data CCC"

res=`./dspam-app -c -u $user -h $home "$data DDD"`

if [[ "$res" == *"Dspam-Classification: spam"* ]]; then
    exit 0
fi
exit 1