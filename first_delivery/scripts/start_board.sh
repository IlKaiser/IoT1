#!/bin/bash 

# start broker
broker_mqtts ../config/rsmb_local.conf &

# build and run
cd ../source/nucleo-f401re
make all clean flash term 
