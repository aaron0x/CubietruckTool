#!/bin/bash

export PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/usr/local/games:/usr/games

if [ $(cat /sys/class/leds/blue\:ph21\:led2/brightness) == "0" ]; then
   echo "1" > /sys/class/leds/blue\:ph21\:led2/brightness
fi

