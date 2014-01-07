#!/usr/bin/env python


import sys
import os
import numpy
import subprocess
import numpy as np	
import signal
import paramiko            					#for ssh
from threading import Thread
import time


import numpy as np
import scipy as sp
import scipy.stats

def mean_confidence_interval(data, confidence=0.95):

    a = 1.0*np.array(data)
    n = len(a)
    m, se = np.mean(a), scipy.stats.sem(a)
    h = se * sp.stats.t._ppf((1+confidence)/2., n-1)

    return h


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

		

def run_source(e1, e2, e3, i, rate, app):
	filename = "fs"
	fs=open(filename,'w')
	ssh = paramiko.SSHClient()
	ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
	ssh.connect('rasp04.lab.es.aau.dk', username='pi', password='rasp')
	stdin, fs, stderr = ssh.exec_command("./peyman/nc4rasp --type=source --id=1 --field=binary --destinationID=3 --relayID=3 --max_tx=10000 --host=10.0.0.255 --e1={} --e2={} --e3={} --port=6000 --rate={} --iteration={} --helperflag={}".format(e1, e2, e3, rate,i, app))
	if stderr:
		print ("err src: ", stderr.read())
	
	while True:
		line=fs.readline()
		if not line:
			break
		line=line.split(":")
		if line[0] == "completion_time":
			complete_t.insert( i, float(line[1].strip()) )
	print "compelet time"
	print complete_t
	fs.close()
    

	
def run_relay(e1, e2, e3, estimate, rate, i):

	ssh = paramiko.SSHClient()
	ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
	ssh.connect('rasp05.lab.es.aau.dk', username='pi', password='rasp')
	stdin, stdout, stderr = ssh.exec_command("./peyman/nc4rasp --strategy=credit_base_playncool --type=relay --field=binary --max_tx=1000 --id=2 --destinationID=3 --sourceID=1 --host=10.0.0.255  --e1={} --e2={} --e3={} --rate=50 --estimate={} --port=6000 --iteration={}".format(e1, e2, e3, estimate, i))
	if stderr:
		print ("err rly: ", stderr.read())
	date = stdout.read()
	

def run_destination(e1, e2, e3, i):
	filename = "f"
	f=open(filename,'w')
	ssh = paramiko.SSHClient()
	ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
	ssh.connect('rasp06.lab.es.aau.dk', username='pi', password='rasp')
	stdin, f, stderr = ssh.exec_command("./peyman/nc4rasp --type=destination --max_tx=1000 --e1={} --e2={} --e3={} --port=6000 --helperflag=1 --sourceID=1 --helperID=2 --id=3 --field=binary --iteration={}".format(e1, e2, e3, i))
	if stderr:
		print ("err des: ", stderr.read())	
	while True:
		line=f.readline()
		#	print line
		if not line:
			break

e1 = 50
e2 = 30
e3 = 80

n = 100
x = 1
complete_time_app = list()
complete_time_app_ci = list()
iterations = 10

rate = [2, 5, 10, 20, 30, 50, 70 ]
estimate = 0


for app in range(1, 3):
	completion_time = list()
	completion_time_ci = list()

	for i in range(0, len(rate)):
		complete_t = list(); 
		E1=float (float(e1)/100);
		E3=float(float(e3)/100);
		E2=float (float(e2)/100);
		
		
		for j in range(1,iterations):
		
			
			print "source is started"
			print "relay is started"
			print "helper is started"
			print "rate "
			print rate[i] 
			print "iteration: {}".format(j)
	
		
			source = Thread(target = run_source, args = (e1, e2, e3, j, rate[i], app))
			if app == 1:
				relay = Thread(target = run_relay, args = (e1, e2, e3, estimate, rate[i], j))
			destination = Thread(target = run_destination, args = (e1, e2, e3, j))


			if app == 1:
				relay.start()
			
			destination.start()

			time.sleep(2);          # sleep for two secinds until relay and destination get ready

			source.start()

			source.join()
			if app == 1:
				relay.join()
			destination.join()
		print "mean value"
		completion_time.insert(i, meanstdv(complete_t))
		completion_time_ci.insert(i, mean_confidence_interval(complete_t))

		print completion_time

		
	complete_time_app.insert(app, completion_time)
	complete_time_app_ci.insert(app, completion_time_ci)
        
import matplotlib

matplotlib.use('Agg')
import matplotlib.pyplot as plt2
import pickle

x = np.arange(0, 2000 ,200)
print x
print complete_time_app[0]
print rate


plt2.errorbar(rate,complete_time_app[0], yerr = complete_time_app_ci[0])
plt2.errorbar(rate, complete_time_app[1], yerr = complete_time_app_ci[1])

plt2.legend(['with helper', 'without helper'], loc='upper left')
plt2.xlabel('rate(KB/s)')
plt2.ylabel('completion time (Mili second)')

plt2.savefig('completion_time.pdf')
plt2.show()
f = open("completion_time.txt", "w")
pickle.dump(complete_time_app[0], f)
pickle.dump(complete_time_app[1], f)
pickle.dump(complete_time_app_ci[0], f)
pickle.dump(complete_time_app_ci[1], f)

f.close()




import matplotlib.pyplot as plt3

import pylab as p




gain = [x/y for x, y in zip(complete_time_app[1], complete_time_app[0])]
print gain

fig = p.figure()
ax = fig.add_subplot(1,1,1)
x = np.arange(0,101 ,1)

ax.plot(rate, gain)
ax.set_ylabel('gain')
ax.set_xlabel('Rate')

p.savefig('gain.pdf')

