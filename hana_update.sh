#!/bin/bash

python waf configure --bundle=ALL
python waf build

PATH=$PATH:~/toolchains/arm-unknown-linux-gnueabi/bin/ python waf configure --options=cxx_mkspec=cxx_raspberry_gxx47_arm --bundle=ALL
python waf build
scp build/cxx_raspberry_gxx47_arm/nc4rasp pi@rasp04.lab.es.aau.dk:peyman/nc4rasp
scp build/cxx_raspberry_gxx47_arm/nc4rasp pi@rasp05.lab.es.aau.dk:peyman/nc4rasp
scp build/cxx_raspberry_gxx47_arm/nc4rasp pi@rasp06.lab.es.aau.dk:peyman/nc4rasp
