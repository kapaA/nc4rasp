#pragma once

#include <cstdint>
#include <string>

#include <kodo/rlnc/full_vector_codes.hpp>
#include <kodo/sparse_uniform_generator.hpp>

namespace kodo
{

    /// Implementation of Spars

 template<class Field>
    class sparse_full_rlnc_encoder :
        public // Payload Codec API
               payload_encoder<
               // Codec Header API
               systematic_encoder<
               symbol_id_encoder<
               // Symbol ID API
               plain_symbol_id_writer<
               // Coefficient Generator API
               sparse_uniform_generator<
               // Codec API
               encode_symbol_tracker<
               zero_symbol_encoder<
               linear_block_encoder<
               storage_aware_encoder<
               // Coefficient Storage API

               coefficient_info<
               // Symbol Storage API
               deep_symbol_storage<
               storage_bytes_used<
               storage_block_info<
               // Finite Field API
               finite_field_math<typename fifi::default_field<Field>::type,
               finite_field_info<Field,
               // Factory API
               final_coder_factory_pool<
               // Final type
               sparse_full_rlnc_encoder<Field
                   > > > > > > > > > > > > > > > > >
    { };
}

/*class Rasp
{
    public :
    typedef struct
    {

        int field_size;      // 1 byte
        char density;        // 1 byte
        uint16_t run_number;  // 2 bytes
        uint16_t symbol_size;   // 2 bytes
        uint16_t symbol;      // 2bytes

        std::string destAddress;
        unsigned short destPort;
        char* sendChar;

    } rpiConf;


    typedef enum
    {
	    TX_SUCSESS = 0,
	    TX_FAILURE = 1,

	    CONFIG_SUCSESS = 2,

    } GLOBRET;

};*/

