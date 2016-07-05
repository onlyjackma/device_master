#!/bin/sh
gcc -o master  master.c mqtt_msg.c worker.c mqtt_worker.c -lubox -lubus -lmosquitto -O2 -Wall
