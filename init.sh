#!/bin/bash
#

#
# Compiling static libmosquitto only works on develop branch as of now 
# but develop is fucking unstable, so this is greyed out
#
# git clone https://github.com/eclipse/mosquitto libs/mosquitto
# cd libs/mosquitto
# git checkout develop
# cd ../..

# no need for dependency
# build libevent
git clone https://github.com/libevent/libevent libs/libevent
mkdir libs/libevent/build
cmake -B../libs/libevent/build/ -H../libs/libevent/
