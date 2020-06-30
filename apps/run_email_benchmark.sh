#!/bin/bash

set -eu

source ./testing_preamble.sh

server_name="./email_server"

server="$server_name $port $start_prog $json_file"
client="./email_client localhost $port 1 $padding data/email/input 250"
lockfile="data/email/data/testuser0/testuser0.sdb.lock"

rm -rf $lockfile email_server${logfile_number}.log

launch_server "$server" 4 email_server$logfile_number.log
${time} -f "wall time: %e" $client

killall $(basename $server_name)
wait
