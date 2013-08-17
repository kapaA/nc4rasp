import argparse
import subprocess
import sys
import os
from time import sleep

cmd = [];
run = 0;




def stats(data):
	sum = 0.0
	for value in data:
		sum =sum+ value
	return (sum/len(data))

def meanstdv(x): 
	n, mean, std = len(x), 0, 0 
	print n
	for a in x:
		mean = mean + a 
	mean = mean / float(n)
	return mean


def run_cmd_source(args):
    
    for it in range(1,args.iteration+1):
    
        f=open('termOut.txt','w')
        print "iteration:{}/{} " .format(it,args.iteration)
        src_cmd = ["./rasp","-type=source", "-host=10.0.0.255","-field=binary", "-iteration={}".format(it)] 
        
        print src_cmd
        e = open("errors", "w")
    
        src = subprocess.Popen(src_cmd, stdout=f,  stderr=e,preexec_fn=os.setsid)
        sleep(5.)
        src.terminate()


def run_cmd_destination(args):
    rank_list = [0]*(args.symbol_size+1);
    complete_time_app = list()	
    transmited = list()
    
    tOut=open('termOut.txt','w')
    
    for run in range(1,args.iteration+1):
        print "iteration:{}/{} " .format(run,args.iteration)

        cmd =["./{}".format(args.prog)]
        cmd +=["-field={}".format(args.field)]
        cmd +=["-symbol_size={}".format(args.symbol_size)]
        cmd +=["-symbols={}".format(args.symbols)]
        cmd +=["-host={}".format(args.host)]
        #cmd +=["-port={}".format(args.port)]
        cmd +=["-iteration={}".format(run)]
        cmd +=["-type={}".format(args.role_type)]
        #cmd +=["-max_tx={}".format(args.max_tx)]

        print cmd
        e = open("errors", "w")
        runCmd = subprocess.Popen(cmd, stdout=tOut,  stderr=e);
        # Wait for child process to terminate.
        runCmd.wait(); 
        
        if runCmd.returncode != 0:
            (stdout,stderr) = runCmd.communicate()
            print stderr
            sys.exit(1)
        tOut.close();
        old_rank = 0
        tOut=open('termOut.txt','r')
        while True:
            line=tOut.readline()
            if not line:
                break
            elif (run>=args.iteration):
                break
            line=line.split(":")
            if line[0] == "received_packets":
                complete_time_app.append (float(line[1].strip()))    
            if line[0] == "transmitted":
                transmited.append (int(line[1].strip()))    
            if line[0] == "rank":
                rank = int(line[1].strip())
                if rank == old_rank:
                    rank_list[rank] = rank_list[rank] + 1
                old_rank = rank

    dOut=open('{}_{}_{}_rankList.log'.format(args.nodeID,args.field, args.role_type),'w')
    for item in rank_list:
        dOut.write("%s\n" % item)
    dOut.close
    
    dOut=open('{}_{}_{}_rxPacket.log'.format(args.nodeID,args.field, args.role_type),'w')
    for item in complete_time_app:
        dOut.write("%s\n" % item)
    dOut.close
    
    dOut=open('{}_{}_{}_txPacket.log'.format(args.nodeID,args.field, args.role_type),'w')
    for item in transmited:
        dOut.write("%s\n" % item)
    dOut.close
    
    print "the number of transmited packets "		
    print transmited
    
    print "the number of received packets "
    print complete_time_app
    
    print "the number linear dep"
    print rank_list
    
def main():

    global cmd; 
    global run; 
    
    parser = argparse.ArgumentParser(description="RASP test runner tool.");
    
    parser.add_argument('--nodeID',dest='nodeID',default='raspxx', type=str)
    parser.add_argument('--prog',dest='prog',default='rasp', type=str)
    parser.add_argument('--field',dest='field',default='binary', type=str)
    parser.add_argument('--symbol_size',dest='symbol_size',default=100, type=int)
    parser.add_argument('--symbols',dest='symbols',choices=range(1,1000),default=100,type=int)
    parser.add_argument('--host',dest='host',default="''",type=str)
    parser.add_argument('--port',dest='port',default=5432,type=int)
    parser.add_argument('--iteration',dest='iteration',default=10,type=int)
    parser.add_argument('--type',dest='role_type',choices=['source','destination','relay'],default='source', type=str)
    parser.add_argument('--max_tx',dest='max_tx',default=10000,type=int)
    
    args = parser.parse_args()
    
    cmd = ["sudo "]
    cmd +=["./{}".format(args.prog)]
    cmd +=[" -field={}".format(args.field)]
    cmd +=[" -symbol_size={}".format(args.symbol_size)]
    cmd +=[" -symbols={}".format(args.symbols)]
    cmd +=[" -host={}".format(args.host)]
    cmd +=[" -port={}".format(args.port)]
    cmd +=[" -iteration={}".format(run)]
    cmd +=[" -type={}".format(args.role_type)]
    cmd +=[" -max_tx={}".format(args.max_tx)]

    if('source' == args.role_type):
        run_cmd_source(args);
    elif('destination' == args.role_type):
        run_cmd_destination(args);
    


if __name__ == "__main__":
        main()