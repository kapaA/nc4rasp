#pragma once```````````````````

#include <cstdint>
#include <iostream>           // For cout and cerr
#include <cstdlib>            // For atoi()
#include <vector>
#include <string>

#include "PracticalSocket.h"  // For UDPSocket and SocketException

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
             int estimate)
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
}


void listen_ack(int iteration)
{
	int ackPort = 12345;	
    const int MAXRCVSTRING = 4096; // Longest string to receive
    UDPSocket sock(ackPort);
    char recvString[MAXRCVSTRING + 1]; // Buffer for echo string + \0
    string sourceAddress;              // Address of datagram source
    unsigned short sourcePort;         // Port of datagram source

   while (true)
    {
        try
        {

            int bytesRcvd = sock.recvFrom(recvString, MAXRCVSTRING,
                                          sourceAddress, sourcePort);
                          
			int itr = *((int *)(&recvString[bytesRcvd - 4])); //source ID
            
            if (itr == iteration)
			{
				std::cout << "ACK is received!" << endl;
				finished = true;
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
    UDPSocket sock(destPort);
    char recvString[MAXRCVSTRING + 1]; // Buffer for echo string + \0
    string sourceAddress;              // Address of datagram source
    unsigned short sourcePort;         // Port of datagram source
    int itr, rank;
    vector<size_t> ranks(symbols);
    UDPSocket FW_sock;
	int x = 1;
    int sourceID;
    

    while (!m_decoder->is_complete())
    {
        try
        {
			
			int bytesRcvd = sock.recvFrom(recvString, MAXRCVSTRING,
                                          sourceAddress, sourcePort);
                                          
			sourceID = *((int *)(&recvString[bytesRcvd - 4])); //source ID
            itr = *((int *)(&recvString[bytesRcvd - 8])); //Iteration
            seq = *((int *)(&recvString[bytesRcvd - 12])); //Sequence number

            if (iteration != itr || std::rand ()%100 < E1)
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
	else if (strategy == "hana_heuristic")
		hana_heuristic();

	return 0;

}

int playNcool()
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
	bool flage = false; 
	float e1 = E1 / 100;
	float e2 = E2 / 100;
	float e3 = E3 / 100; 
	
	if (ovear_estimate == 1)
	{
	//e1 = 1 - (0.93 * (1 - e1));		
	e2 = 1 - (0.93 * (1 - e2));		
	//e3 = 1 - (0.93 * (1 - e3));		
	}

	if ((1-e2) < (1-e1)*(e3))
	{
	
		t = (float)(1) / ((1 - e1) * e3);
	
	}
	else
	{
		t = ((float) -symbols * (-1 + e2 + e3 - e1 * e3 ))/(( 2 - e3 - e2 )*( 1 - e1 ) * e3 -(1 - e3) * ( -1 + e2 + e3 - e1 * e3 ));
	    std::cout << "thershold: " << t << std::endl;  
	}
	
	boost::thread th;

	receive_ack = boost::thread(&relay::listen_ack, this, iteration);		// listen to ack packets	

    while (!m_decoder->is_complete())
    {
        try
        {

			if (finished == true)
			{
				std::cout << "The relay is finished" << endl;	
				break;
			}
			
			int bytesRcvd = sock.recvFrom(recvString, MAXRCVSTRING,
                                          sourceAddress, sourcePort);
                                          
			sourceID = *((int *)(&recvString[bytesRcvd - 4])); //source ID
            itr = *((int *)(&recvString[bytesRcvd - 8])); //Iteration
            seq = *((int *)(&recvString[bytesRcvd - 12])); //Sequence number

            if (iteration != itr || std::rand ()%100 < E1)
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

               if (iteration != itr || (std::rand ()%100 < E1 && sourceID == source))
            {
                continue;
            }

            if (std::rand ()%100 < E2 && sourceID != source)
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


void transmit_ack(int iteration)
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










