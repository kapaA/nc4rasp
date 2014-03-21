#!/bin/bash

ssh pi@rasp06.lab.es.aau.dk "killall -q nc4rasp"
ssh pi@rasp04.lab.es.aau.dk "killall -q nc4rasp"
ssh pi@rasp05.lab.es.aau.dk "killall -q nc4rasp"
ssh pi@rasp07.lab.es.aau.dk "killall -q nc4rasp"
ssh pi@rasp11.lab.es.aau.dk "killall -q nc4rasp"

