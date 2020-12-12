#!/bin/bash
# Helper script to terminate processes for CSP and SP
# Caveat: if you have other processes containing "SP" in the name, they will be 
# terminated as well. Use with caution.  

# use space as the awk delimiter, place in a variable
# https://stackoverflow.com/questions/24331687/is-it-possible-to-print-the-awk-output-in-the-same-line
PROCID=`ps -ef | grep SP | grep -v grep | awk 'BEGIN { ORS=" " }; {print $2}'`

echo "Will terminate PIDs: ${PROCID}"

# parse variable using space delimiter into array
read -ra my_array <<< "${PROCID}"

# for each PID in the array, run the kill command
# https://explainshell.com/explain?cmd=kill+-9+5678
# Name     Num   Action    Description
# KILL       9   exit      cannot be blocked
for i in "${my_array[@]}"
do
    kill -s SIGINT $i
done
