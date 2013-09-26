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
             std::string st)
{

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

            if (iteration != itr || std::rand ()%100 + 1 < loss)
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



	int i = 0;
    int interval = 1000/(1000*rate/symbol_size);
    boost::chrono::milliseconds dur(interval);

	while (i < max_tx)
    {
        i++;
        // Encode a packet into the payload buffer
        std::vector<uint8_t> payload(m_decoder->payload_size());
        m_decoder->recode( &payload[0] );
		
        payload.insert(payload.end(), (char *)x, ((char *)&x) + 4);
        payload.insert(payload.end(), (char *)&iteration, ((char *)&iteration) + 4);
		payload.insert(payload.end(), (char *)&id, ((char *)&id) + 4);
					
        if (output == "verbose")
        {
            std::cout << "x:" << (int) seq << std::endl;
            std::cout << "p:" << (int) payload[0] << std::endl;
            std::cout << "iteration:" << (int) iteration << std::endl;
        }

        try
        {
            // Repeatedly send the string (not including \0) to the server
            FW_sock.sendTo((char *)&payload[0], payload.size(), destAddress , destPort);
            boost::this_thread::sleep_for(dur);
        }
        catch (SocketException &e)
        {
            cerr << e.what() << endl;
            exit(0);
        }
    }

	
	
    std::vector<uint8_t> data_out(m_decoder->block_size());
    m_decoder->copy_symbols(sak::storage(data_out));

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
	else 
		playNcool();

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
	if ((1-e2) < (1-e1)*(e3))
	{
	
		t = (float)(1) / ((1 - e1) * e3);
	
	}
	else
	{
		  
		t = ((float) -symbols * (-1 + e2 + e3 - e1 * e3 ))/(( 2 - e3 - e2 )*( 1 - e1 ) * e3 -(1 - e3) * ( -1 + e2 + e3 - e1 * e3 ));
	    std::cout << "thershold: " << t << std::endl;  
	}
	
    while (true)
    {
        try
        {
			
			int bytesRcvd = sock.recvFrom(recvString, MAXRCVSTRING,
                                          sourceAddress, sourcePort);
                                          
			sourceID = *((int *)(&recvString[bytesRcvd - 4])); //source ID
            itr = *((int *)(&recvString[bytesRcvd - 8])); //Iteration
            seq = *((int *)(&recvString[bytesRcvd - 12])); //Sequence number

            if (iteration != itr || std::rand ()%100 + 1 < E1)
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
				boost::thread t(&relay::start_helper, this);	
			
			} 
		}
        catch (SocketException &e)
        {
            cerr << e.what() << endl;
            exit(1);
        }
	}
	
	return 0;
}
 
int start_helper()
{
	
	int x = 1;
	UDPSocket sock;
    int interval = 1000/(1000*rate/symbol_size);
    boost::chrono::milliseconds dur(interval);
    
	while (true)
    {
		
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

	
}

            
};
