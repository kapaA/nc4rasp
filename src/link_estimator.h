#pragma once

#include "PracticalSocket.h"
#include <map>
#include <cstdint>
#include <string>
#include <vector>
#include <boost/shared_ptr.hpp>

// This class estimates the link quality between two nodes
using namespace std;


class linkEstimator 
{

public:
	linkEstimator(int identification,
							 int maximum_tx,
							 int tx_rate,
							 int destinationPort);
	~linkEstimator();


	// for each link the information of the link is stored in the following struct
	struct LinkQualityEntry 
	{
		double last;
		double first;
		double received;
		double lost;
		int last_refresh;	   
		double lost_prob;	   
	};


    UDPSocket rx_sock;
	UDPSocket tx_sock;
	
	int id;
	int max_tx;
	int rate;
	int destPort;
	
public:

	void send_hello();
	void receive_hello();
	void print_result();
	
	std::map<uint32_t , boost::shared_ptr<LinkQualityEntry>> link_quality_table;

};
