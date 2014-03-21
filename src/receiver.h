#pragma once

#include <cstdint>
#include <iostream>           // For cout and cerr
#include <cstdlib>            // For atoi()
#include <vector>
#include <fstream>
#include "link_estimator.h"

#include "PracticalSocket.h"  // For UDPSocket and SocketException

using namespace std;
std::map<uint32_t , boost::shared_ptr<linkEstimator::LinkQualityEntry>> link_quality_table;
int doublicate;


void update_loss(int id, int seq)
{
	if (link_quality_table.find(id) == link_quality_table.end() )
	{
		boost::shared_ptr<linkEstimator::LinkQualityEntry> new_entry(new linkEstimator::LinkQualityEntry);
		std::cout << "new Entry " <<std::endl;
		new_entry->last = seq;
		new_entry->first = seq;
		new_entry->received = 1;
		new_entry->last_refresh = 0;
		
		//boost::shared_ptr<LinkQualityEntry> new_entry1(&new_entry); 
		link_quality_table[id] = new_entry;

	}
	else 
	{
		boost::shared_ptr<linkEstimator::LinkQualityEntry> entry = link_quality_table[id];
		if (seq <= entry->last)
		{
			doublicate++;
			return;
		}
		
  
		entry->received++;
		entry->last = seq;
		entry->lost_prob = (entry->last - entry->first - entry->received + 1)/(entry->last - entry->first + 1) ;
		entry->last_refresh = 0;
		
	}
}

void print_loss_result()
{
	std::map<uint32_t , boost::shared_ptr<linkEstimator::LinkQualityEntry>>::iterator itr = link_quality_table.begin();;
	std::cout << "result:"<< link_quality_table.size() << endl;	

	while (itr != link_quality_table.end())
	{
		std::cout << "ID:"<< itr->first<< endl;	
		std::cout << "loss:"<< itr->second->lost_prob<< endl;
		std::cout << "last:"<< itr->second->last<< endl;
		std::cout << "first:"<< itr->second->first<< endl;
		std::cout << "received:"<< itr->second->received<< endl;
		itr++;	
	}

}

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

    int interval = 1;
    boost::chrono::milliseconds dur(interval);
    UDPSocket sock;
	string ackAddress1 = "172.26.13.255";
	string ackAddress2 = "172.26.13.205";
	int ackPort = 12345;
    int i = 0;
	
	ofstream myfile;
	myfile.open ("example2.txt");
	myfile << "Finished.\n" << iteration;
	myfile.close();
	while (i < 50)
    {
		i++;

		interval = std::rand() % 100;
		boost::chrono::milliseconds dur(interval);

		// Encode a packet into the payload buffer
		std::vector<uint8_t> payload(2);
		payload.insert(payload.end(), (char *)&iteration, ((char *)&iteration) + 4);
		payload.insert(payload.end(), (char *)&id, ((char *)&id) + 4);

		try
		{
		// Repeatedly send the string (not including \0) to the server
    		sock.sendTo((char *)&payload[0], payload.size(), ackAddress1 , ackPort);
    		//sock.sendTo((char *)&payload[0], payload.size(), ackAddress2 , ackPort);
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
            int is_enabledd_helper,
            bool syntetic_loss)
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
    int itr = 0, rank = 0;
    vector<size_t> ranks(symbols);
	int sourceID;
	int recevied_src = 0;
	int recevied_rly = 0;
	int tx_rly = 0;
	int tx_src = 0;

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
    		update_loss(sourceID, seq);

            // drop the packet because of loss
            if (iteration != itr || (std::rand ()%100  < (e3 + ovear_estimate) && sourceID == source && syntetic_loss == true))
            {
                continue;
            }

            if (std::rand ()%100  < (e2 + ovear_estimate) && sourceID == helperID && syntetic_loss == true)
            {
                continue;
            }

		    rank = m_decoder->rank();
		    m_decoder->decode( (uint8_t*)&recvString[0] );

            if (sourceID == source)
            {
				recevied_src++;
				tx_src = seq;				
				cout << "seq_source:" << seq << endl;

            }

            if (sourceID != source)
            {
				recevied_rly++;
				tx_rly = seq;
				cout << "seq_relay:" << seq << endl;
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
				ofstream myfile;
				myfile.open ("rank.txt");
				myfile << "rank.\n" << rank;
				myfile.close();

            }
			
			received_packets++;
            if (rank == m_decoder->rank())  //If rank has not changed the received package is liniar dependent              
				ranks[rank]++; 				//Add a linear dependent cnt to this spot
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
	
	print_loss_result();
    return 0;
}


