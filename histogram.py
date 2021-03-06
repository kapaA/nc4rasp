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


#plt.ion()


iterations = 100
complete_time_app = list()	
transmited = list()	
for app in range(1,iterations):
    f=open(filename,'w')
    f1=open("file2",'w')
    print "iteration:{} " .format(app)

    dst_cmd = ["sudo","./rasp", "-type=receiver", "-field=binary","-iteration={}".format(app)]
    src_cmd = ["sudo","./rasp", "-type=source", "-field=binary", "-rate=2000","-iteration={}".format(app)]
    
    print src_cmd
    e = open("errors", "w")
    e1 = open("errors1", "w")
    
    dst = subprocess.Popen(dst_cmd, stdout=f,  stderr=e)
    #src = subprocess.Popen(src_cmd, stdout=f1,  stderr=e1,preexec_fn=os.setsid)
    
    dst.wait()
   # src.terminate()


    datamin = 99
    datamax = 112
    numbins = 1
    mybins = np.linspace(datamin, datamax, numbins)

    if dst.returncode != 0:
        (stdout,stderr) = dst.communicate()
        print stderr
        sys.exit(1)
    start_time=0
    f.close()
    f = open(filename, "r")
    old_rank = 0
    while True:
        line=f.readline()
        if not line:
            break
        line=line.split(":")
        if line[0] == "received_packets":
            binwidth = 1
            complete_time_app.append (float(line[1].strip()))
        if line[0] == "transmitted":
            transmited.append (int(line[1].strip()))
    #			n, bins, patches = plt.hist(complete_time_app, bins = range(100,110+binwidth,binwidth),  facecolor='green', alpha=0.75)
    #			plt.draw()
        if line[0] == "rank":
        #	print "ramk"
            rank = int(line[1].strip())
            if rank == old_rank:
                rank_list[rank] = rank_list[rank] + 1
            old_rank = rank
print "the number of transmited packets "		
print transmited

print "the number of received packets "
print complete_time_app

print "the number linear dep"
print rank_list

div = np.true_divide(rank_list,iterations)

n, bins, patches = plt.hist(transmited, bins = range(0,400+binwidth,binwidth),  facecolor='green', alpha=0.75)
plt.show()

import pylab as p
#(ax1, ax2) = p.subplots(1, 2, sharey=True)

fig = p.figure()
ax = fig.add_subplot(1,1,1)
x = np.arange(0,101 ,1)

ax.bar(x, div,align='center')
ax.set_ylabel('linear dependent avrage')
ax.set_xlabel('Rank')

ax = fig.add_subplot(1,1,1)
fig.autofmt_xdate()
plt.show()



f.close()
e.close()


