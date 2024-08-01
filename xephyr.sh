#!/bin/bash
make || exit

Xephyr -screen 1280x700 +xinerama :80 &
sleep 0.1
export DISPLAY=:80
./lwm

