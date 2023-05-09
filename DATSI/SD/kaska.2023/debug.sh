#!/usr/bin/bash

export BROKER_PORT=12345

if [ "$(whoami)" = "c200172" ]; then
  export BROKER_HOST="triqui.fi.upm.es"
else
  export BROKER_HOST="localhost"
  
  # Check if port is already in use
  ps -le | grep broker > /dev/null
  if [ $? -eq 0 ]; then
    echo "Port $BROKER_PORT is already in use"
    
    # Kill the process using the specified port
    fuser -k -n tcp $BROKER_PORT > /dev/null 2>&1
    
    if [ $? -eq 0 ]; then
      echo "Killed the process running at port $BROKER_PORT"
    else
      echo "Failed to kill the process running at port $BROKER_PORT"
    fi
  fi
fi

cd broker
make

# Check if make command was successful
if [ $? -ne 0 ]; then
  echo "Make command for BROKER failed with exit code $?"
  exit 1
fi

# Run broker in the background and redirect output to a log file
strace -f -o broker.log ./broker $BROKER_PORT &

cd ../clients
make

# Check if make command was successful
if [ $? -eq 0 ]; then
  if [ "$1" = "-gdb" ]; then
    gdb -x ./test
  elif [ "$1" = "-s" ]; then
    strace -f -o client.log ./test
  elif [ "$1" = "-v" ]; then
    valgrind -s ./test
  elif [ $# -eq 0 ]; then
    ./test
  else
    echo "Invalid parameter. Usage: $0 [-gdb|-s|-v]"
    exit 1
  fi
else
  echo "Make command for CLIENT failed with exit code $?"
fi
