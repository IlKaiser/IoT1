#!/bin/bash 

# start broker
broker_mqtts $PWD/config/rsmb_local.conf &

# build and run
cd source/nucleo-f401re
make all flash term BOARD=nucleo-f401re
