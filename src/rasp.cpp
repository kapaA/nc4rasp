#include "rasp.h"
#include "sender.h"
#include "receiver.h"

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
        int rate = 100;
        int iteration = 0;
        int max_tx = 1000000;

        // Coding options
        string field = "binary";
        int symbols = 100;
        int symbol_size = 100;
        double density = 0.0;

        po::options_description desc("Allowed options");
        desc.add_options()
            ("help", "produce help message")
            ("type", po::value<string>(&type), "node type: source, dest, relay")
            ("host", po::value<string>(&host), "remote host")
            ("port", po::value<int>(&port), "protocol port")
            ("rate", po::value<int>(&rate), "sending rate [kilobytes/sec]")
            ("iteration", po::value<int>(&iteration), "current iteration")
            ("max_tx", po::value<int>(&max_tx), "maximum transmissions")
            ("field", po::value<string>(&field), "field: binary, binary8, binary16")
            ("symbols", po::value<int>(&symbols), "number of symbols")
            ("symbol_size", po::value<int>(&symbol_size), "symbol size")
            ("density", po::value<double>(&density), "coding vector density");

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv , desc), vm);
        po::notify(vm);

        if (vm.count("help"))
        {
            std::cout << desc << std::endl;
            return 1;
        }

        if (type == "source")
        {
            if (field == "binary")
            {
                typedef kodo::sparse_full_rlnc_encoder<fifi::binary> rlnc_encoder;
                send<rlnc_encoder>(host, port, rate, iteration, max_tx,
                                   symbols, symbol_size, density);
            }
            else if (field == "binary8")
            {
                typedef kodo::sparse_full_rlnc_encoder<fifi::binary8> rlnc_encoder;
                send<rlnc_encoder>(host, port, rate, iteration, max_tx,
                                   symbols, symbol_size, density);
            }
            else if (field == "binary16")
            {
                typedef kodo::sparse_full_rlnc_encoder<fifi::binary16> rlnc_encoder;
                send<rlnc_encoder>(host, port, rate, iteration, max_tx,
                                   symbols, symbol_size, density);
            }
        }
        else
        {
            if (field == "binary")
            {
                typedef kodo::full_rlnc_decoder<fifi::binary> rlnc_decoder;
                receive<rlnc_decoder>(port, iteration, symbols, symbol_size);
            }
            else if (field == "binary8")
            {
                typedef kodo::full_rlnc_decoder<fifi::binary8> rlnc_decoder;
                receive<rlnc_decoder>(port, iteration, symbols, symbol_size);
            }
            else if (field == "binary16")
            {
                typedef kodo::full_rlnc_decoder<fifi::binary16> rlnc_decoder;
                receive<rlnc_decoder>(port, iteration, symbols, symbol_size);
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

    return 0;
}
