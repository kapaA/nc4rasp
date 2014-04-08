#pragma once
#include "RawSocket.h"
#include <cstdint>
#include <iostream>           // For cout and cerr
#include <cstdlib>            // For atoi()
#include <vector>
#include <string>
#include <fstream>
#include "PracticalSocket.h"  // For UDPSocket and SocketException
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/bind.hpp>
#include "link_estimator.h"



using namespace std;


template <class Decoder> class relay {

  public:

	typename Decoder::pointer m_decoder;
	int ovear_estimate;
	std::string destAddress;
	int destPort;
	int rate;
	int iteration;
	int symbols;
	int symbol_size;
	int max_tx;
	double E1;
	double E2;
	double E3;
	bool syntetic_loss;
	std::string output;
	int id;
	std::string strategy;
	bool finished ;						// flage showing the destination is done or not
	bool timer_flage;

	boost::thread receive_ack;
	UDPSocket sock;
	RawSocket prom_sock;
	UDPSocket sock_ack;
	int ackPort;
	int source;
	int relayID;
	int helperID;
	int helper_avaliable;
	int destinationID;
	int credit_app;
	bool prom;
	double credit;
	double budget;
	int total_budget;
	boost::mutex mutexQ;
	boost::condition_variable_any condQ;
 	std::map<uint32_t , boost::shared_ptr<linkEstimator::LinkQualityEntry>> link_quality_table;
	int doublicate;
    
relay(std::string dest,
	        int destP,
            int r,
            int it,
            int sym,
            int sym_size,
            int m_tx,
    	    double e_s_r,
	        double e_r_d,
	        double e_s_d,
            bool l,
            std::string out,
            int identification,
            std::string st,
            double estimate,
            int srcID,
            int rlyID,
            int desID,
            int helperID,
            int is_enabled,
            int crd_app,
	        bool promiscuous)
{
	ovear_estimate = estimate;
	destAddress = dest;
	destPort = destP;
	rate = r;
	iteration = it;
	symbols = sym;
	symbol_size = sym_size;
	max_tx = m_tx;
	E1 = e_s_r;
	E2 = e_r_d;
	E3 = e_s_d;
	syntetic_loss = l;
	output = out;
	id = identification;
	strategy = st;
	finished = false;
	sock.setLocalPort(destPort);
	source = srcID;
	relayID = rlyID;
	destinationID = desID;
	helper_avaliable = is_enabled;
	budget = 0;
	ackPort = 12345;
        sock_ack.setLocalPort(ackPort);
	credit_app = crd_app;
	timer_flage = false;
	prom = promiscuous;
//	prom_sock.start("wlan0");
       //RawSocket raw("wlan0");
}

int is_enabled_helper()
{
	return helper_avaliable;
}



void timer()
{

	boost::posix_time::ptime now;
	boost::posix_time::time_duration diff;
	boost::posix_time::ptime start;

	start  = boost::posix_time::microsec_clock::local_time();
	now = boost::posix_time::microsec_clock::local_time();
	diff = start - now;
	boost::chrono::milliseconds dur(100);
    boost::posix_time::microseconds threshold(100000000);
											  
    
        ofstream myfile;
	myfile.open ("diff.txt");
	/*
	while (diff < threshold && timer_flage ==  false )
		if (timer_flage == false)
		{
			std::cerr << "hard stop: " << diff.total_microseconds()<<endl;
			finished = true;
			timer_flage = true;  m 
			sock.close();
			sock_ack.close();
			condQ.notify_one();

		}
		*/
	while (diff < threshold && timer_flage ==  false )
	{
		
		boost::this_thread::sleep_for(dur);
		now = boost::posix_time::microsec_clock::local_time();
		diff =  now - start;
		std::cerr << "diff " <<  diff.total_microseconds() << std::endl;

		myfile << "diff.\n" << diff.total_microseconds();


	}
	if (timer_flage == false)
	{
		std::cerr << "hard stop: " << diff.total_microseconds()<<endl;
		finished = true;
		timer_flage = true;
		sock_ack.close();
		myfile << "closed socket" << diff.total_microseconds();

	}
	myfile << "finished at: .\n" << iteration;

	myfile.close();
	
	
	
}



void listen_ack(int iteration)
{
    const int MAXRCVSTRING = 4096; // Longest string to receive
    char recvString[MAXRCVSTRING + 1]; // Buffer for echo string + \0
    string sourceAddress;              // Address of datagram source
    unsigned short sourcePort;         // Port of datagram source

    while (true)
    {
        try
        {

            int bytesRcvd = sock_ack.recvFrom(recvString, MAXRCVSTRING,
                                          sourceAddress, sourcePort);

	    int ACKsource = *((int *)(&recvString[bytesRcvd - 4])); //source ID
	    int itr = *((int *)(&recvString[bytesRcvd - 8])); //iteartion num

            
            if ((itr == iteration && ACKsource == destinationID) || timer_flage == true)
		{	
			timer_flage = true;	
			boost::mutex::scoped_lock lock( mutexQ );
			std::cout << "ACK is received!" << endl;

			finished = true;
			sock.close();
			ofstream myfile;
			myfile.open ("example.txt");
			myfile << "ACK is received!.\n" << iteration;
			myfile.close();
			condQ.notify_one();
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


int forward_simple_credit_base()
{

    const int MAXRCVSTRING = 4096; // Longest string to receive
    int received_packets = 0;
    int seq = 0;
    char recvString[MAXRCVSTRING + 1]; // Buffer for echo string + \0
    string sourceAddress;              // Address of datagram source
    unsigned short sourcePort;         // Port of datagram source
    int itr = 0, rank = 0;
    vector<size_t> ranks(symbols);
    UDPSocket FW_sock;
    int x = 1;
    int sourceID = 0;

    receive_ack = boost::thread(&relay::listen_ack, this, iteration);		// listen to ack packets	
    boost::thread run_time = boost::thread(&relay::timer, this);		// listen to ack packets		
    boost::thread th = boost::thread(&relay::credit_base_helper, this);					// transmit thread	

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

            cout << "rank:" << m_decoder->rank() << endl;

			// filter the packet from the other nodes
	        if (sourceID != source && sourceID != relayID)
            {
	    	    continue;
	        }
	        if (sourceID == helperID && is_enabled_helper () == 0)
	        {
	    	    continue;
	        }

            // drop the packet because of the loss
            if (iteration != itr || (std::rand ()%100 < (E3 + ovear_estimate) && sourceID == source && syntetic_loss == true))
            {
                continue;
            }

            if (iteration != itr || (std::rand ()%100 < (E2 + ovear_estimate) && sourceID == relayID && syntetic_loss == true))
            {
	        	continue;
	         }
            
            
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


            if (output == "verbose")
            {
                cout << "source ID:" << sourceID << endl;
                cout << "rank:" << m_decoder->rank() << endl;
                cout << "seq:" << seq << endl;
                cout << "itr:" << itr << endl;
                cout << "iteration:" << iteration << endl;
	        	cout << "received_from_source:" << recevied_src << endl;
		        cout << "transmit_from_source:" << tx_src << endl;
		        cout << "received_from_relay:" << recevied_rly << endl;
		        cout << "transmit_from_relay:" << tx_rly << endl;
			
	     }
        update_loss(sourceID, seq);
	    rank = m_decoder->rank();
	    m_decoder->decode( (uint8_t*)&recvString[0] );
	    received_packets++;
			
        if (rank != m_decoder->rank())
	    {
	    	boost::mutex::scoped_lock lock( mutexQ );
		    budget += credit; 
		    condQ.notify_one();
			
	    }
		
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
	std::cout << "start helper" << endl;
	boost::thread t(&relay::start_helper, this, x);
	t.join();
	run_time.join();
    
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


int forward_helper_stupid()
{
    const int MAXRCVSTRING = 4096; // Longest string to receive
    int received_packets = 0;
    int seq = 0;
    char recvString[MAXRCVSTRING + 1]; // Buffer for echo string + \0
    string sourceAddress;              // Address of datagram source
    unsigned short sourcePort;         // Port of datagram source
    int itr, rank;
    vector<size_t> ranks(symbols);
    UDPSocket FW_sock;
    int x = 1;
    int sourceID;
    bool helper_is_started = false;

    receive_ack = boost::thread(&relay::listen_ack, this, iteration);		// listen to ack packets
    boost::thread t; // helper thread

    while (!m_decoder->is_complete())
    {

		if (finished == true)
		{
			//sock.close(); Add this to receive ack
			ofstream myfile;
			myfile.open ("example2.txt");
			myfile << "Finished.\n" << iteration;
			myfile.close();
			std::cout << "The relay is finished" << endl;
			break;
		}


		int bytesRcvd = sock.recvFrom(recvString, MAXRCVSTRING,
									  sourceAddress, sourcePort);

		sourceID = *((int *)(&recvString[bytesRcvd - 4])); //source ID
		itr = *((int *)(&recvString[bytesRcvd - 8])); //Iteration

		seq = *((int *)(&recvString[bytesRcvd - 12])); //Sequence number

		if (helper_is_started == false)
		{
		helper_is_started = true;
		boost::thread t(&relay::start_helper, this, 1);
		}

		if (finished == true)
		{
			//sock.close(); Add this to receive ack
			ofstream myfile;
			myfile.open ("example2.txt");
			myfile << "Finished.\n" << iteration;
			myfile.close();
			std::cout << "The relay is finished" << endl;
			break;
		}


		// filter the packet from the other nodes
		if (sourceID != source)
		{
			continue;
		}

		if (iteration != itr || std::rand ()%100 < (E1 + ovear_estimate))
		{
			continue;
		}


		rank = m_decoder->rank();
		m_decoder->decode( (uint8_t*)&recvString[0] );

		if (output == "verbose")
		{
			cout << "rank:" << m_decoder->rank() << endl;
			cout << "seq:" << seq << endl;
			cout << "itr:" << itr << endl;
			cout << "iteration:" << iteration << endl;
			cout << "source ID:" << sourceID << endl;
		}



    }

	std::cout << "start helper" << endl;
	t.join();

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

/// This approach transmits a packet when it receives a packet from the source
// similar to recode and forward approach
int forward_helper()
{

    const int MAXRCVSTRING = 4096; // Longest string to receive
    int received_packets = 0;
    int seq = 0;
    char recvString[MAXRCVSTRING + 1]; // Buffer for echo string + \0
    string sourceAddress;              // Address of datagram source
    unsigned short sourcePort;         // Port of datagram source
    int itr, rank;
    vector<size_t> ranks(symbols);
    UDPSocket FW_sock;
    int x = 1;
    int sourceID;
    receive_ack = boost::thread(&relay::listen_ack, this, iteration);		// listen to ack packets

    while (!m_decoder->is_complete())
    {
        try
        {
			if (finished == true)
			{
				//sock.close(); Add this to receive ack
				ofstream myfile;
				myfile.open ("example2.txt");
				myfile << "Finished.\n" << iteration;
				myfile.close();
				std::cout << "The relay is finished" << endl;
				break;
			}


			int bytesRcvd = sock.recvFrom(recvString, MAXRCVSTRING,
                                          sourceAddress, sourcePort);

			sourceID = *((int *)(&recvString[bytesRcvd - 4])); //source ID
           		itr = *((int *)(&recvString[bytesRcvd - 8])); //Iteration
		        seq = *((int *)(&recvString[bytesRcvd - 12])); //Sequence number

			if (finished == true)
			{
				//sock.close(); Add this to receive ack
				ofstream myfile;
				myfile.open ("example2.txt");
				myfile << "Finished.\n" << iteration;
				myfile.close();
				std::cout << "The relay is finished" << endl;
				break;
			}

			// filter the packet from the other nodes
			if (sourceID != source)
			{
				continue;
			}

	 		if (iteration != itr || std::rand ()%100 < (E1 + ovear_estimate))
           		 {
               			continue;
           		 }

		   	 rank = m_decoder->rank();
		   	 m_decoder->decode( (uint8_t*)&recvString[0] );

		 	if (output == "verbose")
           		 {
              		  cout << "rank:" << m_decoder->rank() << endl;
               		  cout << "seq:" << seq << endl;
              		  cout << "itr:" << itr << endl;
               		  cout << "iteration:" << iteration << endl;
              		  cout << "source ID:" << sourceID << endl;
           		 }

			std::vector<uint8_t> payload(m_decoder->payload_size());
			m_decoder->recode( &payload[0]);

			payload.insert(payload.end(), (char *)&x, ((char *)&x) + 4);
			payload.insert(payload.end(), (char *)&iteration, ((char *)&iteration) + 4);
			payload.insert(payload.end(), (char *)&id, ((char *)&id) + 4);
		   	x++;

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

	std::cout << "start helper" << endl;
	boost::thread t(&relay::start_helper, this, x);
	t.join();

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

int set_credit ()
{
	float e1 = E1 / 100;
	float e2 = E2 / 100;
	float e3 = E3 / 100; 
	
	credit = (float) 1/(1 - e1);
	
	if (strategy == "relay_simple" &&  is_enabled_helper() == 1)
		credit = (float) 1/(1 - e1*e3);	
	
	if (strategy == "relay_simple" &&  is_enabled_helper() == 0)
		credit = (float) 1/(1 - e3);	
	
	
	int t;
	int d_r;

	if ((1-e2) < (1-e1)*(e3))
	{

		t = (float)(1) / ((1 - e1) * e3);

	}
	else
	{
		t = ((float) -symbols * (-1 + e2 + e3 - e1 * e3 ))/(( 2 - e3 - e2 )*( 1 - e1 ) * e3 -(1 - e3) * ( -1 + e2 + e3 - e1 * e3 ));
	}

	d_r = (1 - e3) * t;
	total_budget = (symbols - d_r)/( 2 - ( e2 + e3 ));

}



int forward()

{

	typename Decoder::factory m_decoder_factory(symbols, symbol_size);
         m_decoder = m_decoder_factory.build();

	if (strategy == "simple")
		forward_helper();
	else if (strategy == "playncool")
		playNcool();
	else if (strategy == "credit_base_playncool")
		credit_base_playNcool();
	else if (strategy == "hana_heuristic")
		hana_heuristic();
	else if (strategy == "relay_simple")
		forward_simple_credit_base();
	else if (strategy == "stupid")
		forward_helper_stupid();

	return 0;

}

int playNcool()
{

    const int MAXRCVSTRING = 4096; // Longest string to receive
    int received_packets = 0;
    int seq = 0;
    //UDPSocket sock(destPort);
    char recvString[MAXRCVSTRING + 1]; // Buffer for echo string + \0
    string sourceAddress;              // Address of datagram source
    unsigned short sourcePort;         // Port of datagram source
    int itr, rank;
    vector<size_t> ranks(symbols);
    UDPSocket FW_sock;
	int sourceID;
	int t = 0;                          // thershold for playncool
	bool flage = false;
	float e1 = E1 / 100;
	float e2 = E2 / 100;
	float e3 = E3 / 100;
	int x = 1;

	if ((1-e2) < (1-e1)*(e3))
	{

		t = (float)(1) / ((1 - e1) * e3);

	}
	else
	{
		t = ((float) -symbols * (-1 + e2 + e3 - e1 * e3 ))/(( 2 - e3 - e2 )*( 1 - e1 ) * e3 -(1 - e3) * ( -1 + e2 + e3 - e1 * e3 ));
	    std::cout << "thershold: " << t << std::endl;
	}

	t = t * (1 - e1);
	boost::thread th;

	receive_ack = boost::thread(&relay::listen_ack, this, iteration);		// listen to ack packets

    while (!m_decoder->is_complete())
    {
        try
        {

			if (finished == true)
			{
				//sock.close(); Add this to receive ack
				ofstream myfile;
				myfile.open ("example2.txt");
				myfile << "Finished.\n" << iteration;
				myfile.close();
				std::cout << "The relay is finished" << endl;
				break;
			}

			int bytesRcvd = sock.recvFrom(recvString, MAXRCVSTRING,
                                          sourceAddress, sourcePort);


			if (finished == true)
			{
				//sock.close(); Add this to receive ack
				ofstream myfile;
				myfile.open ("example2.txt");
				myfile << "Finished.\n" << iteration;
				myfile.close();
				std::cout << "The relay is finished" << endl;
				break;
			}


			sourceID = *((int *)(&recvString[bytesRcvd - 4])); //source ID
            itr = *((int *)(&recvString[bytesRcvd - 8])); //Iteration
            seq = *((int *)(&recvString[bytesRcvd - 12])); //Sequence number

			// filter packet if it is not from the source
			if (sourceID != source)
			{
				continue;
			}

            if (iteration != itr || std::rand ()%100 < (E1 + ovear_estimate))
            {
                continue;
            }

            if (output == "verbose")
            {
                cout << "rank:" << m_decoder->rank() << endl;
                cout << "seq:" << seq << endl;
                cout << "itr:" << itr << endl;
                cout << "iteration:" << iteration << endl;
                cout << "source ID:" << sourceID << endl;
            }


		    rank = m_decoder->rank();
		    m_decoder->decode( (uint8_t*)&recvString[0] );

		    if (rank == t && flage == false)
		    {

				flage = true;
				std::cout << "start_helper:" << seq<<endl;
				cout << "threshold:" << m_decoder->rank() << endl;
				th = boost::thread(&relay::start_helper, this, x);

			}
		}
        catch (SocketException &e)
        {
            cerr << e.what() << endl;
            exit(1);
        }
	}
	th.join();
	//receive_ack.join();		// no need to wait
	return 0;
}


int credit_base_playNcool()
{
	set_credit (); 					// set credit for the helper
    const int MAXRCVSTRING = 4096; // Longest string to receive
    int received_packets = 0;
    int seq = 0;
    //UDPSocket sock(destPort);
    char recvString[MAXRCVSTRING + 1]; // Buffer for echo string + \0
    string sourceAddress;              // Address of datagram source
    unsigned short sourcePort;         // Port of datagram source
    int itr, rank = 0;
    vector<size_t> ranks(symbols);
    UDPSocket FW_sock;
	int sourceID = 0;
	int t = 0;                          // thershold for playncool
	bool flage = false;
	float e1 = E1 / 100;
	float e2 = E2 / 100;
	float e3 = E3 / 100;
	bool close_to_source = false;

    int hdr_len = 0;
 /* Enable overhearing the packets*/
    if (prom == true)
    {
        prom_sock.start("wlan0");
        hdr_len = RawSocket::hdrLen();
    }
/*  calculate the credit value */
	if ((1-e2) < (1-e1)*(e3))
	{

		t = (float)(1) / ((1 - e1) * e3);
		t = t * (1 - e1);
		t = t + 10;
		close_to_source = true;
	}
	else
	{
		t = ((float) -symbols * (-1 + e2 + e3 - e1 * e3 ))/(( 2 - e3 - e2 )*( 1 - e1 ) * e3 -(1 - e3) * ( -1 + e2 + e3 - e1 * e3 ));
	    std::cout << "thershold: " << t << std::endl;
		t = t * (1 - e1);
	}

  	boost::thread th;
	int counter = -100;
	bool flag_counter = false;
	receive_ack = boost::thread(&relay::listen_ack, this, iteration);		// listen to ack packets
	boost::thread run_time = boost::thread(&relay::timer, this);		// listen to ack packets
    int  bytesRcvd = 0; 
    while (!m_decoder->is_complete())
    {
        try
        {

			if (finished == true)
			{
				//sock.close(); Add this to receive ack
				ofstream myfile;
				myfile.open ("example2.txt");
				myfile << "Finished.\n" << iteration;
				myfile.close();
				std::cout << "The relay is finished" << endl;

				break;
			}

            if (prom == false)
                bytesRcvd = sock.recvFrom(recvString, MAXRCVSTRING,
                                          sourceAddress, sourcePort);
            else
                bytesRcvd = prom_sock.recvFrom(recvString, MAXRCVSTRING,
                                          sourceAddress, sourcePort);
            
            char *data = (recvString + hdr_len);



			if (finished == true)
			{
				//sock.close(); Add this to receive ack
				ofstream myfile;
				myfile.open ("example2.txt");
				myfile << "Finished.\n" << iteration;
				myfile.close();
				std::cout << "The relay is finished" << endl;
				break;
			}


			sourceID = *((int *)(&data[bytesRcvd  - 4])); //source ID
            itr = *((int *)(&data[bytesRcvd - 8])); //Iteration
            seq = *((int *)(&data[bytesRcvd - 12])); //Sequence number

			// filter packet if it is not from the source
			if (sourceID != source)
			{
				continue;
			}

            update_loss(sourceID, seq);

            if (iteration != itr || (std::rand ()%100 < (E1 + ovear_estimate) && syntetic_loss == true))
            {
                continue;
            }


		    rank = m_decoder->rank();
		    m_decoder->decode( (uint8_t*)&data[0] );

            if (flage == true && rank != m_decoder->rank())
			{
				boost::mutex::scoped_lock lock( mutexQ );
				budget += credit;
				condQ.notify_one();
				// adding extra credit at the end
				if (close_to_source == true)
				{
					if (m_decoder->rank() == 50 && flag_counter == false)
					{
						flag_counter = true;
						credit = credit*1.25;
						counter = 30 ;
					}
					if (counter > 0)
					{
						counter--;
					}
					else if(counter == 0)
					{
						credit = credit/1.25;
						counter = -100;
					}
				}

			}

            if (output == "verbose")
            {
                cout << "rank:" << m_decoder->rank() << endl;
                cout << "seq:" << seq << endl;
                cout << "itr:" << itr << endl;
                cout << "iteration:" << iteration << endl;
                cout << "source ID:" << sourceID << endl;
                cout << "budget:" << budget << endl;
                cout << "credit:" << credit << endl;
            	ofstream myfile;
				myfile.open ("rank.txt");
				myfile << "rank.\n" << rank;
				myfile.close();
            }

		    if (m_decoder->rank() >= t && flage == false)
		    {

				flage = true;
				std::cout << "start_helper:" << seq<<endl;
				cout << "threshold:" << m_decoder->rank() << endl;
				th = boost::thread(&relay::credit_base_helper, this);

			}
		}
        catch (SocketException &e)
        {
            cerr << e.what() << endl;
            exit(1);
        }
	}
	th.join();
	run_time.join();
	//receive_ack.join();		// no need to wait
	return 0;
}


int hana_heuristic()
{

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
	int sourceID;
	int t = 0;                          // thershold for playncool
	int source = 1;                     // source ID is one
	bool flage = false;
	int x = 1;
	float e1 = E1 / 100;
	float e2 = E2 / 100;
	float e3 = E3 / 100;




	t = (float)(symbols) * (1 - e1)/ (1 - (e1 * e3));
	cout << "Threshold:" << "  " << t << endl;

	boost::thread th;

    while (!m_decoder->is_complete())
    {
        try
        {

			int bytesRcvd = sock.recvFrom(recvString, MAXRCVSTRING,
                                          sourceAddress, sourcePort);

			sourceID = *((int *)(&recvString[bytesRcvd - 4])); //source ID
            itr = *((int *)(&recvString[bytesRcvd - 8])); //Iteration
            seq = *((int *)(&recvString[bytesRcvd - 12])); //Sequence number

               if (iteration != itr || (std::rand ()%100 < (E1 + ovear_estimate) && sourceID == source))
            {
                continue;
            }

            if (std::rand ()%100 < (E2 + ovear_estimate) && sourceID != source)
            {
                continue;
            }

            if (sourceID != source)
			{
				t = t + 1;
			}

            if (output == "verbose" && !m_decoder->is_complete())
            {
                cout << "rank:" << m_decoder->rank() << endl;
                cout << "seq:" << seq << endl;
                cout << "itr:" << itr << endl;
                cout << "iteration:" << iteration << endl;
                cout << "source ID:" << sourceID << endl;
            }


		    rank = m_decoder->rank();
		    m_decoder->decode( (uint8_t*)&recvString[0] );

		    if (rank == t && flage == false)
		    {

				flage = true;
				std::cout << "start helper" << endl;
				th = boost::thread(&relay::start_helper, this, x);

			}
		}
        catch (SocketException &e)
        {
            cerr << e.what() << endl;
            exit(1);
        }
	}
	th.join();

	return 0;
}

void start_helper(int x)
{

	UDPSocket sock;
    int interval = 1000/(1000*rate/symbol_size);
    boost::chrono::milliseconds dur(interval);
    int i = 0;

	while (i < max_tx && finished == false)
    {
        i++;

        // Encode a packet into the payload buffer
        std::vector<uint8_t> payload(m_decoder->payload_size());
		m_decoder->recode( &payload[0]);

		payload.insert(payload.end(), (char *)&x, ((char *)&x) + 4);
		payload.insert(payload.end(), (char *)&iteration, ((char *)&iteration) + 4);
		payload.insert(payload.end(), (char *)&id, ((char *)&id) + 4);
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
    sock_ack.close();
	if (finished == true)
	{
		std::cout << "The relay received ACK from destination" << endl;
	}

}
  
void credit_base_relay()
{
	
    int x = 1;
    UDPSocket sock;
    int interval = 1000/(1000*rate/symbol_size);
    boost::chrono::milliseconds dur(interval);
    int i = 0;
    
    while (m_decoder->is_comeplete() == false && finished == false)
    {
       	boost::mutex::scoped_lock lock(mutexQ);
       
        while( budget <= 0 && finished == false) // while - to guard agains spurious wakeups
        {
            condQ.wait(lock);
        }
    
		if (finished == true)
		{
			std::cout << "The relay received ACK from destination" << endl;
			break;
		}   
				
		budget--;
		i++;
		
		// Encode a packet into the payload buffer
		std::vector<uint8_t> payload(m_decoder->payload_size());
		m_decoder->recode( &payload[0]);
	
		payload.insert(payload.end(), (char *)&x, ((char *)&x) + 4);
		payload.insert(payload.end(), (char *)&iteration, ((char *)&iteration) + 4);
		payload.insert(payload.end(), (char *)&id, ((char *)&id) + 4);
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
}
  
void credit_base_helper()
{

    int x = 1;
    UDPSocket sock;
    int interval = 1000/(1000*rate/symbol_size);
    boost::chrono::milliseconds dur(interval);
    int i = 0;

    while (i < max_tx && finished == false)
    {
       	boost::mutex::scoped_lock lock(mutexQ);

        while( budget <= 0 && finished == false) // while - to guard agains spurious wakeups
        {
            condQ.wait(lock);
        }
		
	
	if (finished == true)
	{
		std::cout << "The relay received ACK from destination" << endl;
		break;
	}

	if (total_budget < 0)
	{
		std::cout << "The budget is finished!"<< endl;
		break;
	}


	total_budget--;
	budget--;
	i++;

	// Encode a packet into the payload buffer
	std::vector<uint8_t> payload(m_decoder->payload_size());
	m_decoder->recode( &payload[0]);

	payload.insert(payload.end(), (char *)&x, ((char *)&x) + 4);
	payload.insert(payload.end(), (char *)&iteration, ((char *)&iteration) + 4);
	payload.insert(payload.end(), (char *)&id, ((char *)&id) + 4);
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

	print_loss_result();
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

};
