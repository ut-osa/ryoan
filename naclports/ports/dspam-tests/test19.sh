#! /bin/bash
# test null handling
file="input/test19.dat"
user="testuser19"

res="$(./dspam-app -c -u $user -i $file 2>&1)"

if [[ "$res" == *"In operation classify: The operation itself has failed"* ]]; then
    exit 0
fi
exit 1
