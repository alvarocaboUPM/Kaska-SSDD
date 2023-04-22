#!/usr/bin/bash

export BROKER_PORT=12345
export BROKER_HOST=localhost

cd broker
make
./broker $BROKER_PORT &

cd ../clients
make
./test