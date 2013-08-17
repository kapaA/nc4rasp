#!/usr/bin/env python


import sys
import os
import pylab
import numpy
import subprocess
import matplotlib.pyplot as plt
import matplotlib.pyplot as plt2
import numpy as np
import signal

def stats(data):
	sum = 0.0
	for value in data:
		sum =sum+ value
	return (sum/len(data))

def meanstdv(x): 
	from math import sqrt 
	n, mean, std = len(x), 0, 0 
	print n
	for a in x:
		mean = mean + a 
	mean = mean / float(n)
	return mean

print sys.argv[1]

rank_list = [0]*101
filename=sys.argv[1]

not_increased ="1"

figFormat = "ps"

iterations = int(sys.argv[2])


for it in range(1,iterations):
    
    f=open(filename,'w')
    print "iteration:{} " .format(it)
    src_cmd = ["./rasp", "-type=source", "-field=binary", "-rate=1000","-max_tx=500","-iteration={}".format(it)]
    
    print src_cmd
    e = open("errors", "w")

    src = subprocess.Popen(src_cmd, stdout=f,  stderr=e,preexec_fn=os.setsid)
    src.wait()


