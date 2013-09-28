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

		
tx_source = list();

def run_source(e1, e2, e3):

	ssh = paramiko.SSHClient()
	ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
	ssh.connect('rasp01.lab.es.aau.dk', username='pi', password='rasp')
	stdin, stdout, stderr = ssh.exec_command("./peyman/nc4rasp --type=source --max_tx=500 --host=255.255.255.255 --e1={} --e2={} --e3={}".format(e1, e2, e3))
	date = stdout.read()

	
def run_relay(e1, e2, e3):

	ssh = paramiko.SSHClient()
	ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
	ssh.connect('rasp02.lab.es.aau.dk', username='pi', password='rasp')
	stdin, stdout, stderr = ssh.exec_command("./peyman/nc4rasp --strategy=playncool --type=relay --max_tx=500 --id=2 --host=rasp04.lab.es.aau.dk --e1={} --e2={} --e3={}".format(e1, e2, e3))
	date = stdout.read()
	#print(date)
	

def run_destination(e1, e2, e3, i):
	filename = "f"
	f=open(filename,'w')

	ssh = paramiko.SSHClient()
	ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
	ssh.connect('rasp04.lab.es.aau.dk', username='pi', password='rasp')
	stdin, f, stderr = ssh.exec_command("./peyman/nc4rasp --type=destination --max_tx=500 --e1={} --e2={} --e3={}".format(e1, e2, e3))
	#data = f.read()
	#print data
	
	while True:
		line=f.readline()
		if not line:
			break
		line=line.split(":")
		if line[0] == "tx":
			tx_source.insert( i, float(line[1].strip()) )
	print "total transmited from source"
	print tx_source 


for i in range(1,20):

	source = Thread(target = run_source, args = (50, 50, 80))
	relay = Thread(target = run_relay, args = (50, 50, 80))
	destination = Thread(target = run_destination, args = (50, 50, 80, i))
	
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
print meanstdv(tx_source)
	

print "hi"

