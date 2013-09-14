#pragma once

#include <cstdint>
#include <iostream>           // For cout and cerr
#include <cstdlib>            // For atoi()
#include <vector>
#include <string>


#include "PracticalSocket.h"  // For UDPSocket and SocketException

using namespace std;



template<class Decoder>
int forward(std::string destAddress,
			int destPort,
            int rate,
            int iteration,
            int symbols,
            int symbol_size,
            int max_tx,
            double loss,
            string &output)
{
    typename Decoder::pointer m_decoder;
    typename Decoder::factory m_decoder_factory(symbols, symbol_size);

    m_decoder = m_decoder_factory.build();

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
    
    while (!m_decoder->is_complete())
    {
        try
        {
            int bytesRcvd = sock.recvFrom(recvString, MAXRCVSTRING,
                                          sourceAddress, sourcePort);

            itr = *((int *)(&recvString[bytesRcvd - 4])); //Iteration
            seq = *((int *)(&recvString[bytesRcvd - 8])); //Sequence number

			if (iteration != itr || std::rand ()%100 + 1 < loss)
            {
                continue;
            }
            
            if (output == "verbose") {
                cout << "rank:" << m_decoder->rank() << endl;
                cout << "seq:" << seq << endl;
                cout << "itr:" << itr << endl;
                cout << "iteration:" << iteration << endl;
            }
			

		    rank = m_decoder->rank();
		    m_decoder->decode( (uint8_t*)&recvString[0] );
		      
			std::vector<uint8_t> payload(m_decoder->payload_size());
			m_decoder->recode( &payload[0]);
		    
		
			payload.insert(payload.end(), (char *)&seq, ((char *)&seq) + 4);
			payload.insert(payload.end(), (char *)&iteration, ((char *)&iteration) + 4);
		    
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
		seq++;
        payload.insert(payload.end(), (char *)&seq, ((char *)&seq) + 4);
        payload.insert(payload.end(), (char *)&iteration, ((char *)&iteration) + 4);
		
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
