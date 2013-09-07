#include "ebson11.h"
#include <sstream>

namespace {

void encode_sample(ebson11::Encoder& encoder) 
{
    // the code below encodes the following json
    /// {
    ///     "int_field" : 150,
    ///     "bool_field" : true,
    ///     "string_field" : "hello world!",
    ///     "nested_obj" : {
    ///         "name" : "My Name",
    ///         "ints" : [ 0,1,2,3,4,5,6,7,8,9 ]
    ///     }
    /// }

    encoder.restart();
    
    encoder.encode_int32( 150, "int_field" ); // integer field
    encoder.encode_bool( true, "bool_field" ); // boolean field
    encoder.encode_string( "hello world!", "string_field" ); // string field
    
    { // nested object 
        ebson11::DocumentGuard raii(encoder, false, "nested_obj"); // starting nested non-array object by the name "nested_obj"
        encoder.encode_string( "my name", "name" ); // adding field string "name" to the "nested_obj"
        
        { // adding an array field
            ebson11::DocumentGuard raii( encoder, true, "ints" ); // adding an array field "ints" to "nested_obj"

            // adding a bunch of integers to the nested_array
            for( int i=0; i< 10; ++i ) 
            {
                std::stringstream sstr;
                sstr << i;
                encoder.encode_int32(i, sstr.str().c_str());
            }
        }
    }
}

} // anon namespace

int main( int argc, char* argv[]) 
{
    ebson11::Encoder encoder;
    encode_sample(encoder);
    const auto& b = encoder.finalize();
    for( const uint8_t* i = b.begin(), *i_end = b.end(); i< i_end; ++i ) {
        printf( "0x%x", *i );
    }

    return 0;
}
