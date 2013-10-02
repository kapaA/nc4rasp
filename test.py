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

		

def run_source(e1, e2, e3):

	ssh = paramiko.SSHClient()
	ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
	ssh.connect('rasp05.lab.es.aau.dk', username='pi', password='rasp')
	stdin, stdout, stderr = ssh.exec_command("./peyman/nc4rasp --type=source  --field=binary8 --max_tx=1000 --host=10.0.0.255 --e1={} --e2={} --e3={} --rate=50 --port=6000".format(e1, e2, e3))
	date = stdout.read()

	
def run_relay(e1, e2, e3):

	ssh = paramiko.SSHClient()
	ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
	ssh.connect('rasp06.lab.es.aau.dk', username='pi', password='rasp')
	stdin, stdout, stderr = ssh.exec_command("./peyman/nc4rasp --strategy=playncool --type=relay --field=binary8 --max_tx=1000 --id=2 --host=rasp07.lab.es.aau.dk --e1={} --e2={} --e3={} --rate=50 --port=6000".format(e1, e2, e3))
	date = stdout.read()
	#print(date)
	

def run_destination(e1, e2, e3, i):
	filename = "f"
	f=open(filename,'w')

	ssh = paramiko.SSHClient()
	ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
	ssh.connect('rasp07.lab.es.aau.dk', username='pi', password='rasp')
	stdin, f, stderr = ssh.exec_command("./peyman/nc4rasp --type=destination --max_tx=1000 --e1={} --e2={} --e3={} --port=6000".format(e1, e2, e3))
	#data = f.read()
	#print data
	
	while True:
		line=f.readline()
		if not line:
			break
		line=line.split(":")
		if line[0] == "transmit_from_source":
			tx_source.insert( i, float(line[1].strip()) )
		if line[0] == "transmit_from_relay":
			tx_relay.insert( i, float(line[1].strip()) )
	print "total transmited from source"
	print tx_source 
	print tx_relay 

e1 = 50
e2 = 0
e3 = 80
n = 100
x = 1
analis_total = list()
tx = list()
tx_no_helper = list(); 

for i in range(1,8):
	
	tx_source = list();
	tx_relay = list(); 

	e2 = e2 + 10

	E1=float (float(e1)/100);
	E3=float(float(e3)/100);
	E2=float (float(e2)/100);
	tx_no_helper.insert(i, n/(1-E3) )		


	if (1-E2) < (1-E1)*(E3):
		t_s=(1)/((1-E1)*E3)
		print "ts"
		print t_s
		d_r=t_s*(1-E3)
		analis_total.insert(i,t_s*x + ((2 *(n - d_r))/(2-(E2+E3)))*((x+1)/2))
		print t_s*x + ((2 *(n - d_r))/(2-(E2+E3)))*((x+1)/2)

	else: 
		print "ts1"
		print E1
		print (1-E2) 
		print 1-E3
		t_s= (-n*(-1+E2+E3-E1*E3))/((2-E3-E2)*(1-E1)*E3 -(1-E3)*(-1+E2+E3-E1*E3))
		print t_s
		d_r=t_s*(1-E3)
		
		print "d_r"
		print d_r
		analis_total.insert(i,t_s*x + ((2 *(n - d_r))/(2-(E2+E3)))*((x+1)/2))
		

	for j in range(1,100):
				
		source = Thread(target = run_source, args = (e1, e2, e3))
		relay = Thread(target = run_relay, args = (e1, e2, e3))
		destination = Thread(target = run_destination, args = (e1, e2, e3, i))
		
		print "source is started"
		print "relay is started"
		print "helper is started"
		relay.start()
		destination.start()
		source.start()

		source.join()
		relay.join()
		destination.join()



	print "mean value"
	tx.insert(i, meanstdv(tx_source) + meanstdv(tx_relay) )
	

"""
import matplotlib.pyplot as plt

x = np.arange(0.1,0.8 ,0.1)
print tx
print analis_total
print tx_no_helper
print "x"
print x

plt.plot(x,tx)
plt.plot(x,analis_total)
plt.plot(x,tx_no_helper)

plt.legend(['implementation result', 'Analysis result', 'without helper'], loc='upper left')
plt.xlabel('e_2')
plt.ylabel('total number of transmission')
plt.show()
"""


print "analisis"
print analis_total

print "result"
print tx
