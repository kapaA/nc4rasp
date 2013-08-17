#include <cstdlib>
#include <gflags/gflags.h>
#include "rasp.h"
#include "sender.h"
#include "receiver.h"
#include <kodo/rlnc/full_vector_codes.hpp>
#include "boost/shared_ptr.hpp"
#include "boost/random.hpp"

using namespace std;

DEFINE_double(density, 0, "");
DEFINE_string(field, "", "binary, binary8, binary16");
DEFINE_int32(symbol_size, 100, "");
DEFINE_int32(symbols, 100, "");
DEFINE_string(host, "localhost", "");
DEFINE_int32(port, 5423, "");
DEFINE_int32(iteration, 5423, "");
DEFINE_int32(rate, 100, "the rate");
DEFINE_string(type, "source", "source, dest, relay");
DEFINE_int32(max_tx, 1000000, "");

int main(int argc, char *argv[]) {
   
    google::ParseCommandLineFlags(&argc, &argv, true);
    string field(FLAGS_field);
    string type(FLAGS_type);
    double density(FLAGS_density);
	std::srand(std::time(0));
    //std::srand(100);

    if (type.compare("source") == 0)
    {
        
        if (field.compare("binary") == 0)
        {

            typedef kodo::sparse_full_rlnc_encoder<fifi::binary> rlnc_encoder;
            Sender <rlnc_encoder> tx;
            tx.send();
                 
        }
        else if (field.compare("binary8") == 0)
        {

            typedef kodo::sparse_full_rlnc_encoder<fifi::binary8> rlnc_encoder;
            Sender <rlnc_encoder> tx;
            tx.send();

        }
        else if (field.compare("binary16") == 0)
        {
            
            typedef kodo::sparse_full_rlnc_encoder<fifi::binary16> rlnc_encoder;
            Sender <rlnc_encoder> tx;
            tx.send();

        }


    }
    else
    {
            
        if (field.compare("binary") == 0)
        {
            
            typedef kodo::full_rlnc_decoder<fifi::binary> rlnc_decoder;
            Receiver <rlnc_decoder> rx;
            rx.receive ();
            
        }
        else if (field.compare("binary8") == 0)
        {
            
            typedef kodo::full_rlnc_decoder<fifi::binary8> rlnc_decoder;
            Receiver <rlnc_decoder> rx;
            rx.receive ();

        }
        else if (field.compare("binary16") == 0)
        {
            
            typedef kodo::full_rlnc_decoder <fifi::binary16> rlnc_decoder;
            Receiver <rlnc_decoder> rx;
            rx.receive ();
            
        }
        
    }

    return EXIT_SUCCESS;
}
