#ifndef SENDER
#define SENDER
#include <gflags/gflags.h>
#include <kodo/rlnc/full_vector_codes.hpp>

#include "PracticalSocket.h"  // For UDPSocket and SocketException
#include <iostream>           // For cout and cerr
#include <cstdlib>            // For atoi()
#include <vector>
#include <chrono>
#include <thread>
#include <stdint.h>
#include <kodo/rlnc/full_vector_codes.hpp>
#ifdef WIN32
#include <windows.h>          // For ::Sleep()
void sleep(unsigned int seconds) {::Sleep(seconds * 1000);}
#else
#include <unistd.h>           // For sleep()	
#endif

using namespace std;

DECLARE_string(host);
DECLARE_int32(port);
DECLARE_int32(symbol_size);
DECLARE_int32(symbols);
DECLARE_double(density);
DECLARE_int32(rate);
DECLARE_int32(iteration);
DECLARE_int32(max_tx);

template<class Encoder>
class Sender  
{
	int symbols;
    int symbol_size ;
	string destAddress ;         // First arg:  destination address
	int rate;
	int max_tx;
	int iteration;
	unsigned short destPort ;  // Second arg: destination port
	double density;
	typename Encoder::pointer m_encoder;
    typename Encoder::factory m_encoder_factory;
  
 public:
    Sender() :
        symbols(FLAGS_symbols),
        symbol_size(FLAGS_symbol_size),
        destAddress(FLAGS_host),
        destPort(FLAGS_port),
        density(FLAGS_density),
        rate(FLAGS_rate),
        iteration(FLAGS_iteration),
        max_tx(FLAGS_max_tx),
        m_encoder_factory(symbols, symbol_size)
    {
        m_encoder = m_encoder_factory.build();

        if (density > 0)
           m_encoder->set_density(density);
		
        m_encoder->set_systematic_off();
        m_encoder->seed(time(0));
        
    }
    
	int send();
};
#endif

template<class T>
int Sender<T>::send()
{
    
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
    int interval = 1000/(1024*rate/symbol_size/8);
    chrono::milliseconds dur (interval);
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
            this_thread::sleep_for(dur);
            
         } catch (SocketException &e) 
         {
             
            cerr << e.what() << endl;
            exit(0);
            
         }

    }

    return 0;
}
