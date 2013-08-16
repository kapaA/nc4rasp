#ifndef  RECEVER
#define RECEVER
#include <kodo/rlnc/full_vector_codes.hpp>
#include <gflags/gflags.h>
#include "PracticalSocket.h"  // For UDPSocket and SocketException
#include <iostream>           // For cout and cerr
#include <cstdlib>            // For atoi()
#include <vector>
#include <stdint.h>
#include <kodo/rlnc/full_vector_codes.hpp>
#ifdef WIN32
#include <windows.h>          // For ::Sleep()
void sleep(unsigned int seconds) {::Sleep(seconds * 1000);}
#else
#include <unistd.h>           // For sleep()	
#endif

using namespace std;


DECLARE_int32(port);
DECLARE_int32(symbol_size);
DECLARE_int32(symbols);
DECLARE_int32(iteration);

template<class Decoder>
class Receiver  
{


 public:
	int symbols;
    int symbol_size ;
    int iteration ;
    /// The encoder factory
	unsigned short destPort ;  // Second arg: destination port
	
  	typename Decoder::pointer m_decoder;
    typename Decoder::factory m_decoder_factory;
    
 public:
     Receiver() :
        symbols(FLAGS_symbols),
        symbol_size(FLAGS_symbol_size),
        destPort(FLAGS_port),
        iteration(FLAGS_iteration),
        m_decoder_factory(symbols, symbol_size)
        {
        m_decoder = m_decoder_factory.build();
        }
	int receive();
	void set_decoder(int sym , int size,unsigned short port);

};
#endif

template<class T>
int Receiver<T>::receive()
{

    const int MAXRCVSTRING = 4096; // Longest string to receive
    int received_packets = 0;
    int out = 0;
	UDPSocket sock(destPort);                
	char recvString[MAXRCVSTRING + 1]; // Buffer for echo string + \0
	string sourceAddress;              // Address of datagram source
	unsigned short sourcePort;         // Port of datagram source
	int itr;
    while (!m_decoder->is_complete()) 
    {
        try 
        {
            
            int bytesRcvd = sock.recvFrom(recvString, MAXRCVSTRING, sourceAddress, sourcePort);
            
            itr = *((int *)(&recvString[bytesRcvd - 4]));
            out = *((int *)(&recvString[bytesRcvd - 8]));
            
			if (iteration != itr)
				{
				continue;   
				}
				
            cout << "rank:" << m_decoder->rank() << endl;
            cout << "seq:" << out << endl;
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

	cout << "rank:" << m_decoder->rank() << endl;
	cout << "seq:" << out << endl;
	cout << "itr:" << itr << endl;
	cout << "iteration:" << iteration << endl;
    
    std::vector<uint8_t> data_out(m_decoder->block_size());
    m_decoder->copy_symbols(sak::storage(data_out));
    
    std::cout << "last_transmitted_seq_num:"<< out << std::endl;
    cout << "received_packets:" << received_packets++ << endl;
  
    return 0;
		
}



