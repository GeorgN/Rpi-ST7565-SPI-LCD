#!/bin/sh
# Shell script to help in installing Pigpio
# http://abyz.co.uk/rpi/pigpio/download.html
#
# Author: boseji <prog.ic@live.in>
#
#  License:
#  This work is licensed under a Creative Commons 
#  Attribution-ShareAlike 4.0 International License.
#  http://creativecommons.org/licenses/by-sa/4.0/
#
#
wget abyz.co.uk/rpi/pigpio/pigpio.zip
unzip pigpio.zip
cd PIGPIO
make
make install
cd ..
sudo rm -rf PIGPIO
