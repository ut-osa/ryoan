#! /bin/bash
# check that libdspam still errors in the same way
user="testuser18"

res="$(./dspam-app -c -u $user "" 2>&1)"

if [[ "$res" == *"In operation classify: The operation itself has failed"* ]]; then
    exit 0
fi
exit 1
