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

		

def run_source(e1, e2, e3, i):
    filename = "fs"
    fs=open(filename,'w')
    ssh = paramiko.SSHClient()
    ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    ssh.connect('rasp05.lab.es.aau.dk', username='pi', password='rasp')
    stdin, fs, stderr = ssh.exec_command("./peyman/nc4rasp --type=source  --field=binary --max_tx=10000 --host=10.0.0.255 --e1={} --e2={} --e3={} --rate=50 --port=6000".format(e1, e2, e3))
   
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

	
def run_relay(e1, e2, e3, estimate):

	ssh = paramiko.SSHClient()
	ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
	ssh.connect('rasp06.lab.es.aau.dk', username='pi', password='rasp')
	stdin, stdout, stderr = ssh.exec_command("./peyman/nc4rasp --strategy=playncool --type=relay --field=binary --max_tx=1000 --id=2 --host=rasp07.lab.es.aau.dk --e1={} --e2={} --e3={} --rate=50 --estimate={} --port=6000".format(e1, e2, e3, estimate))
	date = stdout.read()
	#print(date)
	

def run_destination(e1, e2, e3, i):
    filename = "f"
    f=open(filename,'w')
    ssh = paramiko.SSHClient()
    ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    ssh.connect('rasp07.lab.es.aau.dk', username='pi', password='rasp')
    stdin, f, stderr = ssh.exec_command("./peyman/nc4rasp --type=destination --max_tx=1000 --e1={} --e2={} --e3={} --port=6000 --field=binary".format(e1, e2, e3))
    #data = f.read()
    #print data
    old_rank = 0
    old_rank_relay = 0
    while True:
        line=f.readline()
        if not line:
            break
        line=line.split(":")
        if line[0] == "transmit_from_source":
            tx_s = float(line[1].strip()) 
        if line[0] == "received_from_source":
            rx_s = float(line[1].strip()) 
        if line[0] == "received_from_relay":
            rx_r = float(line[1].strip()) 
        if line[0] == "transmit_from_relay":
            tx_r = float(line[1].strip()) 
        if line[0] == "rank":
            rank = int(line[1].strip())
            if rank == old_rank:
                rank_list[rank] = rank_list[rank] + 1
            old_rank = rank
        if line[0] == "rank_relay":
            rank_relay = int(line[1].strip())
            if rank_relay == old_rank_relay:
                rank_list_relay[rank] = rank_list_relay[rank] + 1
            old_rank_relay = rank_relay
            
    tx_source.insert( i, tx_s)   
    rx_source.insert( i, rx_s)
    rx_relay.insert( i, rx_r)
    tx_relay.insert( i, tx_r)

    print "total transmited from source"
    print tx_source 
    print tx_relay
    print "rank list"
    print rank_list
    
    print "rank list relay"
    print rank_list_relay
        
    print rx_r + rx_s
    f.close()


e1 = 50
e2 = 0
e3 = 80
n = 100
x = 1
tx_app = list()
complete_time_app = list()
rank_app = list()
rank_app_relay = list()
iterations = 10

for app in range(1,3):
    analis_total = list()
    tx = list()
    #tx_no_helper = list(); 
    completion_time = list()
    estimate = 0
    e2=0
    rank = list()
    rank_relay = list()

    for i in range(1,8):
		tx_source = list();
		rx_source = list();
		tx_relay = list();
		rx_relay = list();
		complete_t = list(); 
		rank_list = [0]*101     
		rank_list_relay = [0]*101     
		e2 = e2 + 10
		E1=float (float(e1)/100);
		E3=float(float(e3)/100);
		E2=float (float(e2)/100);
		#tx_no_helper.insert(i, n/(1-E3) )		


		if (1-E2) < (1-E1)*(E3):
			t_s=(1)/((1-E1)*E3)
			print "ts"
			print t_s
			d_r=t_s*(1-E3)
		   # analis_total.insert(i,t_s*x + ((2 *(n - d_r))/(2-(E2+E3)))*((x+1)/2))
		   # print t_s*x + ((2 *(n - d_r))/(2-(E2+E3)))*((x+1)/2)

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
			#analis_total.insert(i,t_s*x + ((2 *(n - d_r))/(2-(E2+E3)))*((x+1)/2))
			
        
		for j in range(1,iterations):
					
			source = Thread(target = run_source, args = (e1, e2, e3, i))
			if app == 1:
				relay = Thread(target = run_relay, args = (e1, e2, e3, estimate))
			destination = Thread(target = run_destination, args = (e1, e2, e3, i))
			
			print "source is started"
			print "relay is started"
			print "helper is started"
			print "e2, j"
			print e2
			print j
			
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
		tx.insert(i, meanstdv(tx_source) + meanstdv(tx_relay) )
		completion_time.insert(i, meanstdv(complete_t))
		print tx
		print completion_time
		rank.insert(i,  rank_list)
		rank_relay.insert(i,  rank_list_relay)

    complete_time_app.insert(app, completion_time)
    rank_app.insert(app,rank)
    rank_app_relay.insert(app,rank_relay)
   #div = np.true_divide(rank_list,iterations)

    tx_app.insert(app, tx)
        



import matplotlib.pyplot as plt

x = np.arange(0.1,0.8 ,0.1)
print tx_app[0]
print tx_app[1]

plt.plot(x,tx_app[0])
plt.plot(x,tx_app[1])
#plt.plot(x,analis_total)

plt.legend(['with helper', 'without helper', 'without helper'], loc='upper left')
plt.xlabel('e_2')
plt.ylabel('total number of transmission')
plt.savefig('tx.pdf')
plt.show()

f = open("linear-dep-e2=20.txt", "w")
f.write("with helper:{}\n".format(tx_app[0]))
f.write("without helper:{}\n".format(tx_app[1]))
f.close()


#################################################################3
print "rank"
print rank_app[0][1]
print rank_app_relay[0][1]

div = np.true_divide(rank_app[0][1], iterations - 1)
div2 = np.true_divide(rank_app_relay[0][1], iterations - 1)

print div
import pylab as p
fig = p.figure()
ax = fig.add_subplot(1,1,1)
x = np.arange(0,101 ,1)

ax.bar(x, div,align='center')
ax.bar(x, div2,align='center', color ='red')
ax.set_ylabel('linear dependent avrage')
ax.set_xlabel('Rank')

plt.legend(['received from relay and source', 'received from Relay'], loc='upper left')

ax = fig.add_subplot(1,1,1)
fig.autofmt_xdate()
p.savefig('linear-dep-e2=20.pdf')
p.show()

f = open("linear-dep-e2=20.txt", "w")
f.write("with helper:{}\n".format(rank_app[0][1]))
f.close()

###########################################################
print "rank"
print rank_app[0][4]
print rank_app_relay[0][4]


div = np.true_divide(rank_app[0][4], iterations - 1)
div2 = np.true_divide(rank_app_relay[0][4], iterations - 1)

print div
import pylab as p
fig = p.figure()
ax = fig.add_subplot(1,1,1)
x = np.arange(0,101 ,1)

ax.bar(x, div,align='center')
ax.bar(x, div2,align='center', color ='red')
ax.set_ylabel('linear dependent avrage')
ax.set_xlabel('Rank')

plt.legend(['received from relay and source', 'received from Relay'], loc='upper left')

ax = fig.add_subplot(1,1,1)
fig.autofmt_xdate()
p.savefig('linear-dep-e2=50.pdf')
p.show()
f = open("linear-dep-e2=50.txt", "w")
f.write("with helper:{}\n".format(rank_app[0][4]))
f.close()


#############################################################
print "rank"
print rank_app[0][6]
print rank_app_relay[0][6]


div = np.true_divide(rank_app[0][6], iterations - 1)
div2 = np.true_divide(rank_app_relay[0][6], iterations - 1)

print div
import pylab as p
fig = p.figure()
ax = fig.add_subplot(1,1,1)
x = np.arange(0,101 ,1)

ax.bar(x, div,align='center')
ax.bar(x, div2, align='center', color ='red')
ax.set_ylabel('linear dependent avrage')
ax.set_xlabel('Rank')

plt.legend(['received from relay and source', 'received from Relay'], loc='upper left')

ax = fig.add_subplot(1,1,1)
fig.autofmt_xdate()
p.savefig('linear-dep-e2=70.pdf')
p.show()

f = open("linear-dep-e2=70.txt", "w")
f.write("with helper:{}\n".format(rank_app[0][6]))
f.close()

