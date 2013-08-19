#pragma once

#include <cstdint>
#include <iostream>           // For cout and cerr
#include <cstdlib>            // For time()
#include <string>
#include <vector>

#include <boost/chrono.hpp>
#include <boost/thread.hpp>

#include "PracticalSocket.h"  // For UDPSocket and SocketException


template<class Encoder>
int send(std::string destAddress,
         int destPort,
         int rate,
         int iteration,
         int max_tx,
         int symbols,
         int symbol_size,
         double density)
{
    typename Encoder::pointer m_encoder;
    typename Encoder::factory m_encoder_factory(symbols, symbol_size);

    m_encoder = m_encoder_factory.build();

    if (density > 0)
        m_encoder->set_density(density);

    m_encoder->set_systematic_off();
    m_encoder->seed((uint32_t)time(0));

	cout << "sender:" <<  endl;

    // Allocate some data to encode. In this case we make a buffer
    // with the same size as the encoder's block size (the max.
    // amount a single encoder can encode)

    std::vector<uint8_t> data_in(m_encoder->block_size());

    // Just for fun - fill the data with random data
    for(auto &e : data_in)
        e = 'P';

    // Assign the data buffer to the encoder so that we may start
    // to produce encoded symbols from it
    m_encoder->set_symbols(sak::storage(data_in));
    int x = 1;
    int interval = 1000/(1000*rate/symbol_size);
    boost::chrono::milliseconds dur(interval);
    UDPSocket sock;

    int i = 0;
    while (i < max_tx)
    {
        i++;
        // Encode a packet into the payload buffer
        std::vector<uint8_t> payload(m_encoder->payload_size());
        m_encoder->encode( &payload[0] );

        payload.insert(payload.end(), (char *)&x, ((char *)&x) + 4);
        payload.insert(payload.end(), (char *)&iteration, ((char *)&iteration) + 4);
        std::cout << "x:" << (int) x << std::endl;
        std::cout << "p:" << (int) payload[0] << std::endl;
        std::cout << "iteration:" << (int) iteration << std::endl;
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

    return 0;
}
