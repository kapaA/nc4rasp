#pragma once

#include <cstdint>
#include <iostream>           // For cout and cerr
#include <cstdlib>            // For atoi()
#include <vector>


#include "PracticalSocket.h"  // For UDPSocket and SocketException

using namespace std;

void print_result(vector<size_t> &ranks, size_t rx, size_t seq)
{
    cout << "{" << endl;
    cout << "  'ranks': [";
    for (auto i : ranks)
        cout << i << ",";
    cout << "]," << endl;
    cout << "  'received_packets': " << rx << "," << endl;
    cout << "  'last_transmitted_seq_num': " << seq << "," << endl;
    cout << "}" << endl;
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


template<class Decoder>
int receive(int destPort,
            int iteration,
            int symbols,
            int symbol_size,
			double e1,
			double e2,
			double e3,
			string &output,
            int id,
            double ovear_estimate,
            int source,
            int helperID,
            int is_enabledd_helper)
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
	int sourceID;
	int recevied_src;
	int recevied_rly;
	int tx_rly;
	int tx_src;

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
			if (sourceID != source && sourceID != helperID)
			{
				continue;
			}

			if (sourceID == helperID && is_enabledd_helper == 0)
			{
				continue;	
			}
            std::cout << "here 2" << std::endl;
            // drop the packet because of loss
            if (iteration != itr || (std::rand ()%100  < (e3 + ovear_estimate) && sourceID == source))
            {
                continue;
            }

            if (std::rand ()%100  < (e2 + ovear_estimate) && sourceID == helperID)
            {
                continue;
            }

		    rank = m_decoder->rank();
		    m_decoder->decode( (uint8_t*)&recvString[0] );

            if (sourceID == source)
            {
				recevied_src++;
				tx_src = seq;
            }

            if (sourceID != source)
            {
				recevied_rly++;
				tx_rly = seq;
                cout << "rank_relay:" << m_decoder->rank() << endl;

            }

            if (output == "verbose" ) {
                cout << "source ID:" << sourceID << endl;
                cout << "rank:" << m_decoder->rank() << endl;
                cout << "seq:" << seq << endl;
                cout << "itr:" << itr << endl;
                cout << "iteration:" << iteration << endl;
                cout << "e3:" << e3 << endl;
				cout << "received_from_source:" << recevied_src << endl;
				cout << "transmit_from_source:" << tx_src << endl;
				cout << "received_from_relay:" << recevied_rly << endl;
				cout << "transmit_from_relay:" << tx_rly << endl;
            }
			

		    //received_packets++;
			
			//cout << "rank: " << rank << endl; //DEBUG!
			//cout << "m_decoder->rank(): " << m_decoder->rank() << endl;
			//cout << "decoder_completion: " << m_decoder->is_complete() << endl; //DEBUG!
			//cout << "seq:" << seq << endl; //DEBUG!
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



