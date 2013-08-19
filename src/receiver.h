#pragma once

#include <cstdint>
#include <iostream>           // For cout and cerr
#include <cstdlib>            // For atoi()
#include <vector>


#include "PracticalSocket.h"  // For UDPSocket and SocketException

using namespace std;

template<class Decoder>
int receive(int destPort,
            int iteration,
            int symbols,
            int symbol_size)
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
	int itr;

    while (!m_decoder->is_complete())
    {
        try
        {
            int bytesRcvd = sock.recvFrom(recvString, MAXRCVSTRING,
                                          sourceAddress, sourcePort);

            itr = *((int *)(&recvString[bytesRcvd - 4]));
            seq = *((int *)(&recvString[bytesRcvd - 8]));

			if (iteration != itr)
			{
				continue;
            }

            cout << "rank:" << m_decoder->rank() << endl;
            cout << "seq:" << seq << endl;
            cout << "itr:" << itr << endl;
            cout << "iteration:" << iteration << endl;

			m_decoder->decode( (uint8_t*)&recvString[0] );
            received_packets++;

        }
        catch (SocketException &e)
        {
            cerr << e.what() << endl;
            exit(1);
        }
    }

    std::cout << "ITERATION FINISHED: "<< iteration << std::endl;

    std::vector<uint8_t> data_out(m_decoder->block_size());
    m_decoder->copy_symbols(sak::storage(data_out));

    std::cout << "last_transmitted_seq_num:"<< seq << std::endl;
    std::cout << "received_packets:" << received_packets++ << endl;

    return 0;
}