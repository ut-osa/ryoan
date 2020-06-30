#!/bin/bash

set -eu

source ./testing_preamble.sh

server_name="./image_server"

server="$server_name $port $start_prog $json_file"
client="./image_client localhost $port 1 data/images/input/"

# FIXME 5 should be 6, one module isn't printing
launch_server "$server" 5 image_server$logfile_number.log
${time} -f "wall time: %e" $client

killall $(basename $server_name)
wait
