#include "rasp.h"
#include "sender.h"
#include "receiver.h"
#include "relay.h"
#include "link_estimator.h"

#include <boost/program_options.hpp>

using namespace std;

namespace po = boost::program_options;

int main(int argc, char *argv[])
{
    srand((uint32_t)time(0));

    try
    {
        // Network options
        string type = "source";
        string host = "localhost";
        int port = 5423;
        int rate = 5;
        int iteration = 0;
        int max_tx = 10000;
		int id = 1;
		std::string strategy = "simple";
		double e1 = 0;
		double e2 = 0;
		double e3 = 0;
		
        // Coding options
        string field = "binary";
        int symbols = 100;
        int symbol_size = 100;
        double density = 0.0;
        string output = "verbose";
		bool syntetic_loss = true;
		double ovear_estimate = 0;
		int sourceID = 1;
		int relayID = 2;
		int helperID = 1000;
		int DestinationID = 2;
		int helper_flag = 0;
		int credit_app = 2;
		bool link_quality = false;
        po::options_description desc("Allowed options");
        desc.add_options()
            ("help", "produce help message")
            ("type", po::value<string>(&type), "node type: source, dest, relay")
            ("strategy", po::value<string>(&strategy), "strategy simple, playncool, hana_heuristic")
            ("host", po::value<string>(&host), "remote host")
            ("port", po::value<int>(&port), "protocol port")
            ("estimate", po::value<double>(&ovear_estimate), "estimate the error")
            ("rate", po::value<int>(&rate), "sending rate [kilobytes/sec]")
            ("iteration", po::value<int>(&iteration), "current iteration")
            ("max_tx", po::value<int>(&max_tx), "maximum transmissions")
            ("helperflag", po::value<int>(&helper_flag), "1 to enable helper")
            ("field", po::value<string>(&field), "field: binary, binary8, binary16")
            ("symbols", po::value<int>(&symbols), "number of symbols")
            ("symbol_size", po::value<int>(&symbol_size), "symbol size")
            ("id", po::value<int>(&id), "node ID")
            ("sourceID", po::value<int>(&sourceID), "source ID")
            ("relayID", po::value<int>(&relayID), "relay ID")
            ("destinationID", po::value<int>(&DestinationID), "DestinationID")
            ("helperID", po::value<int>(&helperID), "helperID")
            ("creditapp", po::value<int>(&credit_app), "helper credit app")
            ("density", po::value<double>(&density), "coding vector density")
            ("synteticLoss", po::value<bool>(&syntetic_loss), "syntatic loss is true by defult")
            ("e1", po::value<double>(&e1), "error between source and relay")
            ("e2", po::value<double>(&e2), "error between relay and destination")
            ("e3", po::value<double>(&e3), "error between source and destination")
            ("format", po::value<string>(&output), "output format: verbose, python")
            ("linkquality", po::value<bool>(&link_quality), "enable the link quality mesurment");

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv , desc), vm);
        po::notify(vm);
		std::srand((uint32_t)time(0));
        if (vm.count("help"))
        {
            std::cout << desc << std::endl;
            return 1;
        }
        
		if (link_quality == true)
		{
			auto link = new linkEstimator(id, max_tx, rate, port);
			link->send_hello();
			return 1;
		}
		
        if (type == "source")
        {
            if (field == "binary")
            {
                typedef kodo::sparse_full_rlnc_encoder<fifi::binary> rlnc_encoder;
				auto send = new sender<rlnc_encoder>(host, port, rate, iteration, max_tx,
                                   symbols, symbol_size, density, output, id, relayID, DestinationID, helper_flag, e1, e2, e3, syntetic_loss);
				send->transmit();
            }
            else if (field == "binary8")
            {
                typedef kodo::sparse_full_rlnc_encoder<fifi::binary8> rlnc_encoder;
				auto send = new sender<rlnc_encoder>(host, port, rate, iteration, max_tx,
                                   symbols, symbol_size, density, output, id, relayID, DestinationID, helper_flag, e1, e2, e3, syntetic_loss);
				send->transmit();            }
            else if (field == "binary16")
            {
                typedef kodo::sparse_full_rlnc_encoder<fifi::binary16> rlnc_encoder;
                auto send = new sender<rlnc_encoder>(host, port, rate, iteration, max_tx,
                                   symbols, symbol_size, density, output, id, relayID, DestinationID, helper_flag, e1, e2, e3, syntetic_loss);
				send->transmit();
            }
        }
        else if (type == "destination")
        {
            if (field == "binary")
            {
                typedef kodo::full_rlnc_decoder<fifi::binary> rlnc_decoder;
                receive<rlnc_decoder>(port, iteration, symbols, symbol_size, e1, e2, e3, output, id, ovear_estimate, sourceID, helperID, helper_flag, syntetic_loss);
            }
            else if (field == "binary8")
            {
                typedef kodo::full_rlnc_decoder<fifi::binary8> rlnc_decoder;
                receive<rlnc_decoder>(port, iteration, symbols, symbol_size, e1, e2, e3, output, id, ovear_estimate, sourceID, helperID, helper_flag, syntetic_loss);
            }
            else if (field == "binary16")
            {
                typedef kodo::full_rlnc_decoder<fifi::binary16> rlnc_decoder;
                receive<rlnc_decoder>(port, iteration, symbols, symbol_size, e1, e2, e3, output, id, ovear_estimate, sourceID, helperID, helper_flag, syntetic_loss);
            }
        }
        else 
        {
            if (field == "binary")
            {
                typedef kodo::full_rlnc_decoder<fifi::binary> rlnc_decoder;
                auto r = new relay<rlnc_decoder>(host,port, rate, iteration, symbols, symbol_size, max_tx, e1, e2, e3, syntetic_loss, output, id, strategy, ovear_estimate, sourceID, relayID, DestinationID, helperID, helper_flag, credit_app);
                r->forward();
            }
            else if (field == "binary8")
            {
                typedef kodo::full_rlnc_decoder<fifi::binary8> rlnc_decoder;
                auto r = new relay<rlnc_decoder>(host,port, rate, iteration, symbols, symbol_size, max_tx, e1, e2, e3, syntetic_loss, output, id, strategy, ovear_estimate, sourceID, relayID, DestinationID, helperID, helper_flag, credit_app);
                r->forward();
                            }
            else if (field == "binary16")
            {
                typedef kodo::full_rlnc_decoder<fifi::binary16> rlnc_decoder;
                auto r = new relay<rlnc_decoder>(host,port, rate, iteration, symbols, symbol_size, max_tx, e1, e2, e3, syntetic_loss, output, id, strategy, ovear_estimate, sourceID, relayID, DestinationID, helperID, helper_flag, credit_app);
                r->forward();            
             }
        }
    }
    catch (std::exception& e)
    {
        std::cerr << "Error: " << e.what() << "\n";
    }
    catch(...)
    {
        std::cerr << "Exception of unknown type!\n";
    }


	//auto link = new linkEstimator();
	
    return 0;
}
