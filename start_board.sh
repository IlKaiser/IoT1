#!/bin/bash 

# start broker
broker_mqtts $PWD/config/rsmb_local.conf &

# build and run
cd source
make all flash term BOARD=nucleo-f401re
