#!/bin/bash

set -eu

source ./testing_preamble.sh

server_name="./translation_server"
server="$server_name $port $start_prog $json_file"
client="./translation_client localhost $port 1 1 data/translation/input 30"

launch_server "$server" 1 translate_server$logfile_number.log
${time} -f "wall time: %e" $client

killall $(basename $server_name)
wait
