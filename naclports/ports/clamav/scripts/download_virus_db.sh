#!/bin/bash
rm -rf virus_db
mkdir virus_db
cd virus_db
wget http://database.clamav.net/main.cvd
wget http://database.clamav.net/daily.cvd
wget http://database.clamav.net/bytecode.cvd
