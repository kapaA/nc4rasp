#include "PracticalSocket.h"
#include "link_estimator.h"
#include <string>
#include <iostream>
#include <boost/chrono.hpp>
#include <boost/thread.hpp>
#include <assert.h>     /* assert */

using namespace std;

linkEstimator::linkEstimator(int identification,
							 int maximum_tx,
							 int tx_rate,
							 int destinationPort)
{
	id = identification;
	max_tx = maximum_tx;
	rate = tx_rate;
	destPort = destinationPort;
	rx_sock.setLocalPort(destPort);
}

void linkEstimator::send_hello()
{
    int x = 0;
    string destAddress = "10.0.0.255";
    int i = 0;

    int interval = 1/(rate);
    boost::chrono::milliseconds dur(interval);

	boost::thread rx = boost::thread(&linkEstimator::receive_hello, this);

    while (i < max_tx)
	{

        i++;
        std::vector<uint8_t> payload(0);

        payload.insert(payload.end(), (char *)&x, ((char *)&x) + 4);
        payload.insert(payload.end(), (char *)&id, ((char *)&id) + 4);

        x++;
        try
        {
            linkEstimator::tx_sock.sendTo((char *)&payload[0], payload.size(), destAddress , destPort);
            boost::this_thread::sleep_for(dur);
        }
        catch (SocketException &e)
        {
            cerr << e.what() << endl;
            exit(0);
        }
    }
    print_result();
}


void linkEstimator::receive_hello()
{
	const int MAXRCVSTRING = 1500;
	short unsigned int sourcePort = 0;
	string sourceAddress;
	char recvString[MAXRCVSTRING + 1]; // Buffer for echo string + \0
  	int doublicate;

	while (true)
	{
		try
		{
			int bytesRcvd = rx_sock.recvFrom(recvString, MAXRCVSTRING,
											  sourceAddress, sourcePort);
			int id = *(int *)(&recvString[bytesRcvd - 4]);
			int seq = *(int *)(&recvString[bytesRcvd - 8]);

			if (link_quality_table.find(id) == link_quality_table.end() )
			{
				boost::shared_ptr<LinkQualityEntry> new_entry(new LinkQualityEntry);
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
				boost::shared_ptr<LinkQualityEntry> entry = link_quality_table[id];
				if (seq <= entry->last)
				{
					doublicate++;
					continue;
				}


				entry->received++;
				entry->last = seq;
				entry->lost_prob = (entry->last - entry->first - entry->received + 1)/(entry->last - entry->first + 1) ;
				entry->last_refresh = 0;

			}
		}
		catch (SocketException &e)
        {
            cerr << e.what() << endl;
            exit(1);
        }

 	}

}

void linkEstimator::print_result()
{
	std::map<uint32_t , boost::shared_ptr<LinkQualityEntry>>::iterator itr = link_quality_table.begin();;
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
