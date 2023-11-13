#include <cxxopts.hpp>
#include <fmt/core.h>
#include <icecream.hpp>
#include <nlohmann/json.hpp>
#include <sys/stat.h>

#include <bitset>
#include <fstream>

#define VERSION_MAJOR 4
#define VERSION_MINOR 0
#define VERSION_PATCH 0

#define PROGRAM_NAME "cccaster"
#define FOLDER_NAME PROGRAM_NAME
#define FOLDER_PERMISSIONS S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH

using json = nlohmann::json;

int main( const int _argumentCount, const char** _argumentCVector ) {
    std::ofstream l_logFile;

    {
        const std::string l_folderName = FOLDER_NAME;
        const mode_t l_folderPermissions = FOLDER_PERMISSIONS;

        int l_returnCode = mkdir( l_folderName.c_str(), l_folderPermissions );

        l_logFile.open( fmt::format( "{}/log.txt", l_folderName ) );
        icecream::ic.output( l_logFile );

        IC0();
        std::vector< std::string > l_argumentVector(
            _argumentCVector, _argumentCVector + _argumentCount );
        IC( _argumentCount, l_argumentVector );

        std::bitset< 8 > l_folderPermissionsBinary( l_folderPermissions );
        IC( l_folderName, l_folderPermissionsBinary, l_returnCode );
    }

    {
        cxxopts::Options options( std::string( PROGRAM_NAME ),
                                  "Gameplay enhances for MBAACC" );

        options.add_options(
            "",
            { { "r,rollback", "Set default rollback", cxxopts::value< int >() },
              { "p,practice", "Launch practice mode" },
              { "s,spectate", "Launch spectate mode" },
              { "t,tournament", "Launch tournament mode" },
              { "c,cpu", "Launch versus CPU mode" },
              { "v,verbose", "Verbose output" },
              { "d,debug", "Enable debugging" },
              { "h,help", "Display this information." } } );

        auto l_result = options.parse( _argumentCount, _argumentCVector );

        if ( l_result.count( "help" ) ) {
            IC( l_result.count( "help" ) );

            std::cout << options.help() << std::endl;

            return ( 0 );
        }

        if ( l_result.count( "verbose" ) ) {
            IC( l_result.count( "verbose" ) );

            fmt::print( "{}\n", l_result[ "verbose" ].as< bool >() );
        }

        std::ifstream l_jsonFileIn;
        std::string l_fileName;

        if ( l_result.count( "file" ) ) {
            l_fileName =
                "cccaster/" + l_result[ "file" ].as< std::string >() + ".json";
            l_jsonFileIn.open( l_fileName );

        } else {
            l_fileName = fmt::format( "cccaster/v{}.{}.json", VERSION_MAJOR,
                                      VERSION_MINOR );
            l_jsonFileIn.open( l_fileName );
        }

        IC( l_result.count( "file" ), l_fileName, l_jsonFileIn.is_open() );

        json l_data;

        {
            std::ofstream l_jsonFileOut;

            if ( !( l_jsonFileIn.good() ) ) {
                IC( !( l_jsonFileIn.good() ), l_data.empty() );

                l_data[ "integer" ] = 42;

                IC( l_data.dump( 4 ), l_fileName );

                l_jsonFileOut.open( l_fileName );

                if ( l_jsonFileOut.good() ) {
                    l_jsonFileOut << std::setw( 4 ) << l_data << std::endl;

                } else {
                    try {
                        l_jsonFileOut.exceptions( l_jsonFileOut.failbit );

                    } catch (
                        const std::ios_base::failure& _jsonFileOutFailure ) {
                        IC( _jsonFileOutFailure.code(),
                            _jsonFileOutFailure.what() );
                    }
                }
            }

            if ( l_result.count( "integer" ) ) {
                IC( l_result.count( "integer" ) );

                uint32_t l_integer = l_result[ "integer" ].as< int >();

                IC( l_integer, l_data.empty() );

                l_data[ "integer" ] = l_integer;

                IC( l_data.dump( 4 ), l_fileName );

                l_jsonFileOut.open( l_fileName );

                if ( l_jsonFileOut.good() ) {
                    l_jsonFileOut << std::setw( 4 ) << l_data << std::endl;

                } else {
                    try {
                        l_jsonFileOut.exceptions( l_jsonFileOut.failbit );

                    } catch (
                        const std::ios_base::failure& _jsonFileOutFailure ) {
                        IC( _jsonFileOutFailure.code(),
                            _jsonFileOutFailure.what() );
                    }
                }

            } else if ( l_jsonFileIn.good() ) {
                IC( l_jsonFileIn.good() );

                l_data = json::parse( l_jsonFileIn );

                uint32_t l_integer;

                if ( l_data.contains( "integer" ) ) {
                    IC( l_data.contains( "integer" ) );

                    l_integer = l_data[ "integer" ];

                } else {
                    l_integer = -1;
                }

                IC( l_integer );
            }
        }

        IC( static_cast< uint32_t >( l_data[ "integer" ] ) );

        fmt::print( "{}\n", static_cast< uint32_t >( l_data[ "integer" ] ) );

        if ( l_result.count( "debug" ) ) {
            IC( l_result.count( "debug" ) );

            const std::string l_dataDump = l_data.dump();

            IC( l_dataDump );
        }
    }

    return ( 0 );
}
