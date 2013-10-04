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

		

def run_source(e1, e2, e3, i, rate):
    filename = "fs"
    fs=open(filename,'w')
    ssh = paramiko.SSHClient()
    ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    ssh.connect('rasp05.lab.es.aau.dk', username='pi', password='rasp')
    stdin, fs, stderr = ssh.exec_command("./peyman/nc4rasp --type=source  --field=binary --max_tx=10000 --host=10.0.0.255 --e1={} --e2={} --e3={} --port=6000 --rate={}".format(e1, e2, e3, rate))
   
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
    

	
def run_relay(e1, e2, e3, estimate, rate):

	ssh = paramiko.SSHClient()
	ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
	ssh.connect('rasp06.lab.es.aau.dk', username='pi', password='rasp')
	stdin, stdout, stderr = ssh.exec_command("./peyman/nc4rasp --strategy=playncool --type=relay --field=binary --max_tx=1000 --id=2 --host=rasp07.lab.es.aau.dk --e1={} --e2={} --e3={} --estimate={} --port=6000 --rate={}".format(e1, e2, e3, estimate,rate))
	date = stdout.read()
	

def run_destination(e1, e2, e3, i):
    filename = "f"
    f=open(filename,'w')
    ssh = paramiko.SSHClient()
    ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    ssh.connect('rasp07.lab.es.aau.dk', username='pi', password='rasp')
    stdin, f, stderr = ssh.exec_command("./peyman/nc4rasp --type=destination --max_tx=1000 --e1={} --e2={} --e3={} --port=6000 --field=binary".format(e1, e2, e3))
    while True:
			line=f.readline()
		#	print line
			if not line:
				break

e1 = 50
e2 = 50
e3 = 80
n = 100
x = 1
complete_time_app = list()
iterations = 100
rate = 200
estimate = 0
for app in range(1, 3):
    completion_time = list()
   
    for i in range(1,12):
		complete_t = list(); 
		rate = 100 + rate;
		E1=float (float(e1)/100);
		E3=float(float(e3)/100);
		E2=float (float(e2)/100);

		for j in range(1,iterations):
			source = Thread(target = run_source, args = (e1, e2, e3, i, rate))
			if app == 1:
				relay = Thread(target = run_relay, args = (e1, e2, e3, estimate, rate))
			destination = Thread(target = run_destination, args = (e1, e2, e3, i))

			print "source is started"
			print "relay is started"
			print "helper is started"
			print "e2, j"
			print e2
			print j
			print app

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
		print completion_time

        
    complete_time_app.insert(app, completion_time)
        



import matplotlib.pyplot as plt2

import matplotlib
x = np.arange(0, 1100 ,100)
print x
print complete_time_app[0]
plt2.plot(x,complete_time_app[0])
plt2.plot(x,complete_time_app[1])

plt2.legend(['with helper', 'without helper'], loc='upper left')
plt2.xlabel('e_2')
plt2.ylabel('completion time (Micro second)')

plt2.savefig('completion_time.pdf')
plt2.show()
f = open("completion_time.txt", "w")
f.write("with helper:{}\n".format(complete_time_app[0]))
f.write("without helper:{}\n".format(complete_time_app[1]))
f.close()

