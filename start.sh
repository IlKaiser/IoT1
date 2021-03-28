#!/bin/bash 

# start broker
broker_mqtts conf/rsmb_local.conf &

# build and run
cd source
make all clean flash term BOARD=nucleo-f401re
