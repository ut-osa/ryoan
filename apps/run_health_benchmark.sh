#!/bin/bash

set -eu

source testing_preamble.sh

server_name="./health_server"

server="$server_name $port $start_prog $json_file"
client="./health_client localhost $port 10 data/health/bench_data.tst"

launch_server "$server" 3 health_server$logfile_number.log
echo "console bogs down, check output in health_out_$logfile_number.log"
${time} -f "wall time: %e" $client > health_out_$logfile_number.log

killall $(basename $server_name)
wait
