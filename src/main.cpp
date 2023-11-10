#include <cxxopts.hpp>
#include <fmt/core.h>
#include <icecream.hpp>
#include <nlohmann/json.hpp>

#include <fstream>

using json = nlohmann::json;

int main( const int _argumentCount, const char** _argumentCVector ) {
    std::ofstream l_logFile( "log.txt" );
    icecream::ic.output( l_logFile );

    IC0();

    std::vector< std::string > l_argumentVector(
        _argumentCVector, _argumentCVector + _argumentCount );
    IC( _argumentCount, l_argumentVector );

    cxxopts::Options options( "MyProgram",
                              "One line description of MyProgram" );

    options.add_options(
        "", { { "d,debug", "Enable debugging" },
              { "i,integer", "Int param", cxxopts::value< int >() },
              { "f,file", "File name", cxxopts::value< std::string >() },
              { "v,verbose", "Verbose output",
                cxxopts::value< bool >()->default_value( "false" ) } } );

    auto l_result = options.parse( _argumentCount, _argumentCVector );

    if ( l_result.count( "verbose" ) ) {
        fmt::print( "{}\n", l_result[ "verbose" ].as< bool >() );
    }

    if ( l_result.count( "file" ) ) {
        std::ifstream l_jsonFile( l_result[ "file" ].as< std::string >() );
        json l_data = json::parse( l_jsonFile );

        uint32_t l_integer;

        if ( l_result.count( "integer" ) ) {
            l_integer = l_result[ "integer" ].as< int >();

        } else {
            if ( l_data.contains( "integer" ) ) {
                l_integer = l_data[ "integer" ];
            }
        }

        fmt::print( "{}\n", l_integer );

        if ( l_result.count( "debug" ) ) {
            const std::string l_dataDump = l_data.dump();

            IC( l_dataDump );
        }
    }

    return ( 0 );
}
