#pragma once

#include <cstdint>
#include <iostream>           // For cout and cerr
#include <cstdlib>            // For atoi()
#include <vector>
#include <string>
#include <fstream>
#include "PracticalSocket.h"  // For UDPSocket and SocketException
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/bind.hpp>

using namespace std;


template <class Decoder> class relay {
 
  public:

	typename Decoder::pointer m_decoder;
	int ovear_estimate;
	std::string destAddress;
	int destPort;
	int rate;
	int iteration;
	int symbols;
	int symbol_size;
	int max_tx;
	double E1;
	double E2;
	double E3;
	double loss;
	std::string output;
	int id;
	std::string strategy;
	bool finished ;						// flage showing the destination is done or not
	boost::thread receive_ack; 
	UDPSocket sock;
	int source;
	int relayID;
	int helperID;
	int helper_avaliable;
	int destinationID;
	
	double credit;
	double budget;
	
	boost::mutex mutexQ;               
	boost::mutex mutexCond;            // mutex for the condition variable
	boost::condition_variable condQ;
 
 relay(std::string dest,
			int destP,
            int r,
            int it,
            int sym,
            int sym_size,
            int m_tx,
			double e_s_r,
			double e_r_d,
			double e_s_d,
            double l,
            std::string out,
            int identification,
            std::string st,
            double estimate,
            int srcID,
            int rlyID,
            int desID,
            int helperID,
            int is_enabled)
{
	ovear_estimate = estimate;
	destAddress = dest;
	destPort = destP;
	rate = r;
	iteration = it;
	symbols = sym;
	symbol_size = sym_size;
	max_tx = m_tx;
	E1 = e_s_r;
	E2 = e_r_d;
	E3 = e_s_d;
	loss = l;
	output = out;
	id = identification;
	strategy = st;
	finished = false;
	sock.setLocalPort(destPort);
	source = srcID;
	relayID = rlyID;
	destinationID = desID;
	helper_avaliable = is_enabled;
	budget = 0;
}

int is_enabled_helper()
{
return helper_avaliable;
}

void listen_ack(int iteration)
{
	int ackPort = 12345;	
    const int MAXRCVSTRING = 4096; // Longest string to receive
    UDPSocket sock_ack(ackPort);
    char recvString[MAXRCVSTRING + 1]; // Buffer for echo string + \0
    string sourceAddress;              // Address of datagram source
    unsigned short sourcePort;         // Port of datagram source

   while (true)
    {
        try
        {

            int bytesRcvd = sock_ack.recvFrom(recvString, MAXRCVSTRING,
                                          sourceAddress, sourcePort);
                                          
			int ACKsource = *((int *)(&recvString[bytesRcvd - 4])); //source ID                          
			int itr = *((int *)(&recvString[bytesRcvd - 8])); //iteartion num

            
            if (itr == iteration && ACKsource == destinationID)
			{
				std::cout << "ACK is received!" << endl;
				finished = true;
				sock.close();
				ofstream myfile;
				myfile.open ("example.txt");
				myfile << "ACK is received!.\n" << iteration;
				myfile.close();
				condQ.notify_one();	

				break;
			}	
            
         }
        catch (SocketException &e)
        {
            cerr << e.what() << endl;
            exit(1);
        }
 
	}
}


int forward_simple()
{
	
    const int MAXRCVSTRING = 4096; // Longest string to receive
    int received_packets = 0;
    int seq = 0;
    char recvString[MAXRCVSTRING + 1]; // Buffer for echo string + \0
    string sourceAddress;              // Address of datagram source
    unsigned short sourcePort;         // Port of datagram source
    int itr, rank;
    vector<size_t> ranks(symbols);
    UDPSocket FW_sock;
	int x = 1;
    int sourceID;

	receive_ack = boost::thread(&relay::listen_ack, this, iteration);		// listen to ack packets	

    while (!m_decoder->is_complete())
    {
        try
        {
			
			int bytesRcvd = sock.recvFrom(recvString, MAXRCVSTRING,
                                          sourceAddress, sourcePort);
                                          
			sourceID = *((int *)(&recvString[bytesRcvd - 4])); //source ID
            itr = *((int *)(&recvString[bytesRcvd - 8])); //Iteration
            seq = *((int *)(&recvString[bytesRcvd - 12])); //Sequence number
			
			// filter the packet from the other nodes
			if (sourceID != source && sourceID != relayID)
			{
				continue;
			}
			if (sourceID == helperID && is_enabled_helper () == 0)
			{
				continue;	
			}
			
            // drop the packet because of the loss
            if (iteration != itr || (std::rand ()%100 < (E3 + ovear_estimate) && sourceID == source))
            {
                continue;
            }
            
            if (iteration != itr || (std::rand ()%100 < (E2 + ovear_estimate) && sourceID == relayID))
            {
				continue;
			}
            

            if (output == "verbose") {
                cout << "rank:" << m_decoder->rank() << endl;
                cout << "seq:" << seq << endl;
                cout << "itr:" << itr << endl;
                cout << "iteration:" << iteration << endl;
                cout << "source ID:" << sourceID << endl;
            }
						
		    rank = m_decoder->rank();
		    m_decoder->decode( (uint8_t*)&recvString[0] );
		      
			std::vector<uint8_t> payload(m_decoder->payload_size());
			m_decoder->recode( &payload[0]);
		    
			payload.insert(payload.end(), (char *)&x, ((char *)&x) + 4);
			payload.insert(payload.end(), (char *)&iteration, ((char *)&iteration) + 4);
			payload.insert(payload.end(), (char *)&id, ((char *)&id) + 4);
		    x++;
		    
			try
			{
				// Repeatedly send the string (not including \0) to the server
				FW_sock.sendTo((char *)&payload[0], payload.size(), destAddress , destPort);
				//boost::this_thread::sleep_for(dur);
			}
			catch (SocketException &e)
			{
				cerr << e.what() << endl;
				exit(0);
			}
		    
		    
			received_packets++;
			//cout << "received_packets: " << received_packets << endl << endl;		

            if (rank == m_decoder->rank()) //If rank has not changed the received package is liniar dependent              
				ranks[rank]++; //Add a linear dependent cnt to this spot
        }
        catch (SocketException &e)
        {
            cerr << e.what() << endl;
            exit(1);
        }
    }
	
	transmit_ack(itr, id);
	std::cout << "start helper" << endl;
	boost::thread t(&relay::start_helper, this);	
	t.join();
	
    if (output == "verbose")
    {
        std::cout << "ITERATION FINISHED: "<< iteration << std::endl;
        std::cout << "last_transmitted_seq_num:"<< seq << std::endl;
        std::cout << "received_packets:" << received_packets++ << endl;
    }
    else
    {
        print_result(ranks, received_packets, seq);
    }

    return 0;

}            
            

int forward()
            
{
	
	typename Decoder::factory m_decoder_factory(symbols, symbol_size);
    m_decoder = m_decoder_factory.build();

	if (strategy == "simple")
		forward_simple();
	else if (strategy == "playncool") 
		playNcool();
	else if (strategy == "credit_base_playncool") 
		credit_base_playNcool();
	else if (strategy == "hana_heuristic")
		hana_heuristic();

	return 0;

}

int playNcool()
{
				
    const int MAXRCVSTRING = 4096; // Longest string to receive
    int received_packets = 0;
    int seq = 0;
    //UDPSocket sock(destPort);
    char recvString[MAXRCVSTRING + 1]; // Buffer for echo string + \0
    string sourceAddress;              // Address of datagram source
    unsigned short sourcePort;         // Port of datagram source
    int itr, rank;
    vector<size_t> ranks(symbols);
    UDPSocket FW_sock;
	int sourceID;
	int t = 0;                          // thershold for playncool
	bool flage = false; 
	float e1 = E1 / 100;
	float e2 = E2 / 100;
	float e3 = E3 / 100; 
	

	if ((1-e2) < (1-e1)*(e3))
	{
	
		t = (float)(1) / ((1 - e1) * e3);
	
	}
	else
	{
		t = ((float) -symbols * (-1 + e2 + e3 - e1 * e3 ))/(( 2 - e3 - e2 )*( 1 - e1 ) * e3 -(1 - e3) * ( -1 + e2 + e3 - e1 * e3 ));
	    std::cout << "thershold: " << t << std::endl;  
	}
	
	t = t * (1 - e1); 
	boost::thread th;

	receive_ack = boost::thread(&relay::listen_ack, this, iteration);		// listen to ack packets	

    while (!m_decoder->is_complete())
    {
        try
        {

			if (finished == true)
			{
				//sock.close(); Add this to receive ack 
				ofstream myfile;
				myfile.open ("example2.txt");
				myfile << "Finished.\n" << iteration;
				myfile.close();
				std::cout << "The relay is finished" << endl;	
				break;
			}
			
			int bytesRcvd = sock.recvFrom(recvString, MAXRCVSTRING,
                                          sourceAddress, sourcePort);
                                          
			
			if (finished == true)
			{
				//sock.close(); Add this to receive ack 
				ofstream myfile;
				myfile.open ("example2.txt");
				myfile << "Finished.\n" << iteration;
				myfile.close();
				std::cout << "The relay is finished" << endl;	
				break;
			}
			
			
			sourceID = *((int *)(&recvString[bytesRcvd - 4])); //source ID
            itr = *((int *)(&recvString[bytesRcvd - 8])); //Iteration
            seq = *((int *)(&recvString[bytesRcvd - 12])); //Sequence number
			
			// filter packet if it is not from the source
			if (sourceID != source)
			{
				continue;	
			}
            
            if (iteration != itr || std::rand ()%100 < (E1 + ovear_estimate))
            {
                continue;
            }
			
            if (output == "verbose")
            {
                cout << "rank:" << m_decoder->rank() << endl;
                cout << "seq:" << seq << endl;
                cout << "itr:" << itr << endl;
                cout << "iteration:" << iteration << endl;
                cout << "source ID:" << sourceID << endl;
            }
						

		    rank = m_decoder->rank();
		    m_decoder->decode( (uint8_t*)&recvString[0] );
		    
		    if (rank == t && flage == false)
		    {
				
				flage = true;
				std::cout << "start_helper:" << seq<<endl;
				cout << "threshold:" << m_decoder->rank() << endl;
				th = boost::thread(&relay::start_helper, this);	
			
			} 
		}
        catch (SocketException &e)
        {
            cerr << e.what() << endl;
            exit(1);
        }
	}
	th.join();
	//receive_ack.join();		// no need to wait
	return 0;
}
 

int credit_base_playNcool()
{
				
    const int MAXRCVSTRING = 4096; // Longest string to receive
    int received_packets = 0;
    int seq = 0;
    //UDPSocket sock(destPort);
    char recvString[MAXRCVSTRING + 1]; // Buffer for echo string + \0
    string sourceAddress;              // Address of datagram source
    unsigned short sourcePort;         // Port of datagram source
    int itr, rank;
    vector<size_t> ranks(symbols);
    UDPSocket FW_sock;
	int sourceID;
	int t = 0;                          // thershold for playncool
	bool flage = false; 
	float e1 = E1 / 100;
	float e2 = E2 / 100;
	float e3 = E3 / 100; 
	float credit = 1/e1;


	if ((1-e2) < (1-e1)*(e3))
	{
	
		t = (float)(1) / ((1 - e1) * e3);
	
	}
	else
	{
		t = ((float) -symbols * (-1 + e2 + e3 - e1 * e3 ))/(( 2 - e3 - e2 )*( 1 - e1 ) * e3 -(1 - e3) * ( -1 + e2 + e3 - e1 * e3 ));
	    std::cout << "thershold: " << t << std::endl;  
	}
	
	t = t * (1 - e1); 
	boost::thread th;

	receive_ack = boost::thread(&relay::listen_ack, this, iteration);		// listen to ack packets	

    while (!m_decoder->is_complete())
    {
        try
        {

			if (finished == true)
			{
				//sock.close(); Add this to receive ack 
				ofstream myfile;
				myfile.open ("example2.txt");
				myfile << "Finished.\n" << iteration;
				myfile.close();
				std::cout << "The relay is finished" << endl;	
				break;
			}
			
			int bytesRcvd = sock.recvFrom(recvString, MAXRCVSTRING,
                                          sourceAddress, sourcePort);
                                          
			
			if (finished == true)
			{
				//sock.close(); Add this to receive ack 
				ofstream myfile;
				myfile.open ("example2.txt");
				myfile << "Finished.\n" << iteration;
				myfile.close();
				std::cout << "The relay is finished" << endl;	
				break;
			}
			
			
			sourceID = *((int *)(&recvString[bytesRcvd - 4])); //source ID
            itr = *((int *)(&recvString[bytesRcvd - 8])); //Iteration
            seq = *((int *)(&recvString[bytesRcvd - 12])); //Sequence number
			
			// filter packet if it is not from the source
			if (sourceID != source)
			{
				continue;	
			}
            
            if (iteration != itr || std::rand ()%100 < (E1 + ovear_estimate))
            {
                continue;
            }
            
            if (flage == true)
			{
				boost::mutex::scoped_lock lock( mutexQ );
				budget += credit; 
				condQ.notify_one();	
			}
			
            if (output == "verbose")
            {
                cout << "rank:" << m_decoder->rank() << endl;
                cout << "seq:" << seq << endl;
                cout << "itr:" << itr << endl;
                cout << "iteration:" << iteration << endl;
                cout << "source ID:" << sourceID << endl;
                cout << "budget:" << budget << endl;
                cout << "credit:" << credit << endl;
            }
						

		    rank = m_decoder->rank();
		    m_decoder->decode( (uint8_t*)&recvString[0] );
		    
		    if (rank == t && flage == false)
		    {
				
				flage = true;
				std::cout << "start_helper:" << seq<<endl;
				cout << "threshold:" << m_decoder->rank() << endl;
				th = boost::thread(&relay::credit_base_helper, this);	
			
			} 
		}
        catch (SocketException &e)
        {
            cerr << e.what() << endl;
            exit(1);
        }
	}
	th.join();
	//receive_ack.join();		// no need to wait
	return 0;
}
 
 
int hana_heuristic()
{
				
    const int MAXRCVSTRING = 4096; // Longest string to receive
    int received_packets = 0;
    int seq = 0;
    UDPSocket sock(destPort);
    char recvString[MAXRCVSTRING + 1]; // Buffer for echo string + \0
    string sourceAddress;              // Address of datagram source
    unsigned short sourcePort;         // Port of datagram source
    int itr, rank;
    vector<size_t> ranks(symbols);
    UDPSocket FW_sock;
	int sourceID;
	int t = 0;                          // thershold for playncool
	int source = 1;                     // source ID is one
	bool flage = false; 
	
	float e1 = E1 / 100;
	float e2 = E2 / 100;
	float e3 = E3 / 100; 
	
	

	
	t = (float)(symbols) * (1 - e1)/ (1 - (e1 * e3));
	cout << "Threshold:" << "  " << t << endl;

	boost::thread th;
	
    while (!m_decoder->is_complete())
    {
        try
        {
			
			int bytesRcvd = sock.recvFrom(recvString, MAXRCVSTRING,
                                          sourceAddress, sourcePort);
                                          
			sourceID = *((int *)(&recvString[bytesRcvd - 4])); //source ID
            itr = *((int *)(&recvString[bytesRcvd - 8])); //Iteration
            seq = *((int *)(&recvString[bytesRcvd - 12])); //Sequence number

               if (iteration != itr || (std::rand ()%100 < (E1 + ovear_estimate) && sourceID == source))
            {
                continue;
            }

            if (std::rand ()%100 < (E2 + ovear_estimate) && sourceID != source)
            {
                continue;
            }
            
            if (sourceID != source)
			{
				t = t + 1;
			}	
            
            if (output == "verbose" && !m_decoder->is_complete())
            {
                cout << "rank:" << m_decoder->rank() << endl;
                cout << "seq:" << seq << endl;
                cout << "itr:" << itr << endl;
                cout << "iteration:" << iteration << endl;
                cout << "source ID:" << sourceID << endl;
            }
						

		    rank = m_decoder->rank();
		    m_decoder->decode( (uint8_t*)&recvString[0] );
		    
		    if (rank == t && flage == false)
		    {
				
				flage = true;
				std::cout << "start helper" << endl;
				th = boost::thread(&relay::start_helper, this);	
			
			} 
		}
        catch (SocketException &e)
        {
            cerr << e.what() << endl;
            exit(1);
        }
	}
	th.join();
	
	return 0;
}
  
void start_helper()
{
	
	int x = 1;
	UDPSocket sock;
    int interval = 1000/(1000*rate/symbol_size);
    boost::chrono::milliseconds dur(interval);
    int i = 0;
    
	while (i < max_tx && finished == false)
    {
        i++;
        
        // Encode a packet into the payload buffer
        std::vector<uint8_t> payload(m_decoder->payload_size());
		m_decoder->recode( &payload[0]);


	
	
		payload.insert(payload.end(), (char *)&x, ((char *)&x) + 4);
		payload.insert(payload.end(), (char *)&iteration, ((char *)&iteration) + 4);
		payload.insert(payload.end(), (char *)&id, ((char *)&id) + 4);
		x++;

        try
        {
            // Repeatedly send the string (not including \0) to the server
            sock.sendTo((char *)&payload[0], payload.size(), destAddress , destPort);
            boost::this_thread::sleep_for(dur);
        }
        catch (SocketException &e)
        {
            cerr << e.what() << endl;
            exit(0);
        }
    }
    
	if (finished == true)
	{
		std::cout << "The relay received ACK from destination" << endl;
	}
	
}
  
void credit_base_helper()
{
	
	int x = 1;
	UDPSocket sock;
    int interval = 1000/(1000*rate/symbol_size);
    boost::chrono::milliseconds dur(interval);
    int i = 0;
    
	while (i < max_tx && finished == false)
    {
       	boost::mutex::scoped_lock lock(mutexQ);
       
        while( budget <= 0 && finished == false) // while - to guard agains spurious wakeups
        {
            condQ.wait(lock);
        }
    
		if (finished == true)
		{
			std::cout << "The relay received ACK from destination" << endl;
		}   

		budget--;
		i++;
		
		// Encode a packet into the payload buffer
		std::vector<uint8_t> payload(m_decoder->payload_size());
		m_decoder->recode( &payload[0]);
	
		payload.insert(payload.end(), (char *)&x, ((char *)&x) + 4);
		payload.insert(payload.end(), (char *)&iteration, ((char *)&iteration) + 4);
		payload.insert(payload.end(), (char *)&id, ((char *)&id) + 4);
		x++;

		try
		{
			// Repeatedly send the string (not including \0) to the server
			sock.sendTo((char *)&payload[0], payload.size(), destAddress , destPort);
		}
		catch (SocketException &e)
		{
			cerr << e.what() << endl;
			exit(0);
		}
	
    
	}

}
  


void transmit_ack(int iteration, int id)
{

    int interval = 1000/(1000*100/100);
    boost::chrono::milliseconds dur(interval);
    UDPSocket sock;
	string ackAddress = "10.0.0.255";
	int ackPort = 12345;
    int i = 0;
    
    while (i < 5)
    {
        i++;
        // Encode a packet into the payload buffer
        std::vector<uint8_t> payload(2);
		payload.insert(payload.end(), (char *)&iteration, ((char *)&iteration) + 4);
        payload.insert(payload.end(), (char *)&id, ((char *)&id) + 4);

        try
        {
            // Repeatedly send the string (not including \0) to the server
            sock.sendTo((char *)&payload[0], payload.size(), ackAddress , ackPort);
            boost::this_thread::sleep_for(dur);
        }
        catch (SocketException &e)
        {
            cerr << e.what() << endl;
            exit(0);
        }
    }
}


            
};










