#pragma once

#include <cstdint>
#include <iostream>           // For cout and cerr
#include <cstdlib>            // For time()
#include <string>
#include <vector>
#include <boost/chrono.hpp>
#include <boost/thread.hpp>
#include <fstream>

#include "PracticalSocket.h"  // For UDPSocket and SocketException

template <class Encoder> class sender {

  public:

	typename Encoder::pointer m_encoder;
	
	bool finished;
	boost::posix_time::ptime now; 
	boost::posix_time::time_duration diff;
	boost::posix_time::ptime tick;
	std::string destAddress;
	int destPort;
	int rate;
	int iteration;
	int max_tx;
	int symbols;
	int symbol_size;
	double density;
	string output;
	int id;
	int relayID;
	int DestinationID;
	double budget;
	double credit;
	double E1;
	double E2;
	double E3;
	int is_enabled_helper;
	
	sender(std::string destAdd,
         int destP,
         int r,
         int itr,
         int m_tx,
         int sym,
         int sym_size,
         double dens,
         string out,
         int identification,
         int rID,
         int desID,
         int enabled_helper,
    	double e_s_r,
		double e_r_d,
		double e_s_d)
	{
	is_enabled_helper = enabled_helper;
	budget = 0;
	finished = false;
	destAddress = destAdd;
	destPort = destP;
	rate = r;
	iteration = itr;
	max_tx = m_tx;
	symbols = sym;
	symbol_size = sym_size;
	density = dens;
	output = out;
	id = identification;
	relayID = rID;
	DestinationID = desID;
	E1 = e_s_r;
	E2 = e_r_d;
	E3 = e_s_d;
	}
	
void listen_ack(int iteration, int relayID, int DestinationID )
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
                          
			int ACKsource = *((int *)(&recvString[bytesRcvd - 4])); //source ID
			int itr = *((int *)(&recvString[bytesRcvd - 8])); //source ID
            
            if (itr == iteration && ACKsource == relayID)
			{
				std::cout << "ACK is received from relay!" << endl;
				finished = true;
			}	
			if (itr == iteration && ACKsource == DestinationID)
			{
				now  = boost::posix_time::microsec_clock::local_time();
				diff = now - tick;							// completion time 
				std::cout << "ACK is received from destination!" << endl;
				std::cout << "completion_time:" << diff.total_microseconds()<<endl;

				ofstream myfile;
				myfile.open ("example2.txt");
				myfile << "Finished.\n" << iteration;
				myfile.close();

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

int set_credit_helper()
{
	float e1 = E1 / 100;
	float e2 = E2 / 100;
	float e3 = E3 / 100; 
	
	cout << "e1 " << e1 << "e2 " << e2 << "e3 " << e3 << endl;

	credit = (float)1/(1 - e1*e3);
	
	cout << "credit " << credit << endl;
	return 0;	
}

int set_credit_simple()
{
	float e1 = E1 / 100;
	float e2 = E2 / 100;
	float e3 = E3 / 100; 
	
	cout << "e1 " << e1 << "e2 " << e2 << "e3 " << e3 << endl;
	
	credit = (float)1/(1 - e3);

	cout << "credit " << credit << endl;
	return 0;	
}
int transmit ()
{
	if (is_enabled_helper == 1)
		set_credit_helper();
	else 
		set_credit_simple();
	
	transmit_credit();
}

int transmit_simple()
{
    typename Encoder::factory m_encoder_factory(symbols, symbol_size);

    m_encoder = m_encoder_factory.build();

    if (density > 0)
        m_encoder->set_density(density);

    m_encoder->set_systematic_off();
    m_encoder->seed((uint32_t)time(0));

    if (output == "verbose")
        cout << "sender:" <<  endl;

	boost::thread receive_ack = boost::thread(&sender::listen_ack, this, iteration, relayID, DestinationID);	
    
    
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
	
	tick = boost::posix_time::microsec_clock::local_time();

    int i = 0;
    while (i < max_tx)
	{
		if (finished == true)
		{
		std::cout << "The destination is finished" << endl;	
		
		break;
		}

        i++;
        // Encode a packet into the payload buffer
        std::vector<uint8_t> payload(m_encoder->payload_size());
        m_encoder->encode( &payload[0] );

        payload.insert(payload.end(), (char *)&x, ((char *)&x) + 4);
        payload.insert(payload.end(), (char *)&iteration, ((char *)&iteration) + 4);
        payload.insert(payload.end(), (char *)&id, ((char *)&id) + 4);
        
        if (output == "verbose")
        {
            std::cout << "x:" << (int) x << std::endl;
            std::cout << "p:" << (int) payload[0] << std::endl;
            std::cout << "iteration:" << (int) iteration << std::endl;

        }

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
    receive_ack.join(); // wait to receive an ACK from destination
    return 0;
}


int transmit_credit()
{
    typename Encoder::factory m_encoder_factory(symbols, symbol_size);
    m_encoder = m_encoder_factory.build();

    if (density > 0)
        m_encoder->set_density(density);

    m_encoder->set_systematic_off();
    m_encoder->seed((uint32_t)time(0));

    if (output == "verbose")
        cout << "sender:" <<  endl;

	boost::thread receive_ack = boost::thread(&sender::listen_ack, this, iteration, relayID, DestinationID);	
    
    
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
	
	tick = boost::posix_time::microsec_clock::local_time();

    int i = 0;
    while (i < max_tx)
	{
		
        i++;
		if (i < symbols)
		{
		budget += credit;
		}
		else 
		{
		budget = 1;
		}
		
		if (finished == true)
		{
		std::cout << "The destination is finished" << endl;	
		
		break;
		}

		while( budget >= 1)
		{
			budget--;
			// Encode a packet into the payload buffer
			std::vector<uint8_t> payload(m_encoder->payload_size());
			m_encoder->encode( &payload[0] );

			payload.insert(payload.end(), (char *)&x, ((char *)&x) + 4);
			payload.insert(payload.end(), (char *)&iteration, ((char *)&iteration) + 4);
			payload.insert(payload.end(), (char *)&id, ((char *)&id) + 4);
			
			if (output == "verbose")
			{
				std::cout << "x:" << (int) x << std::endl;
				std::cout << "p:" << (int) payload[0] << std::endl;
				std::cout << "iteration:" << (int) iteration << std::endl;
				std::cout << "credit:" << credit << std::endl;
			}

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
	    boost::this_thread::sleep_for(dur);
    }
    receive_ack.join(); // wait to receive an ACK from destination
    return 0;
}


};

