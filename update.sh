#!/bin/bash

python waf configure --bundle=ALL
python waf build

PATH=$PATH:~/toolchains/arm-unknown-linux-gnueabi/bin/ python waf configure --options=cxx_mkspec=cxx_raspberry_gxx47_arm --bundle=ALL
python waf build

ssh rasp06 "killall -q nc4rasp"
scp build/cxx_raspberry_gxx47_arm/nc4rasp rasp05:peyman/nc4rasp
ssh rasp04 "killall -q hana_nc4rasp"
scp build/cxx_raspberry_gxx47_arm/nc4rasp rasp04:peyman/nc4rasp
ssh rasp07 "killall -q hana_nc4rasp"
scp build/cxx_raspberry_gxx47_arm/nc4rasp rasp07:peyman/nc4rasp

ssh rasp11 "killall -q hana_nc4rasp"
scp build/cxx_raspberry_gxx47_arm/nc4rasp rasp11:peyman/nc4rasp



