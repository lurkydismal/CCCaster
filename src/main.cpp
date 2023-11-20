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

#define LOG_FILE_NAME_POSTFIX "log"
#define LOG_FILE_EXTENSION "txt"

#define PREFERENCES_FILE_NAME_POSTFIX "preferences"
#define PREFERENCES_FILE_EXTENSION "json"

using json = nlohmann::json;

const uint8_t g_versionMajor = VERSION_MAJOR;
const uint8_t g_versionMinor = VERSION_MINOR;
const uint8_t g_versionPatch = VERSION_PATCH;

enum gamemode_t { ONLINE, SPECTATE, TOURNAMENT, PRACTICE, CPU };

struct preferences_t {
    int rollback = 3;
    int delay = 0;
    gamemode_t gamemode = PRACTICE;
    bool verbose = true;
    bool debug = false;

    friend void to_json( json& _json, const preferences_t& _preferences ) {
        _json = json{ { "rollback", _preferences.rollback },
                      { "delay", _preferences.delay },
                      { "gamemode", _preferences.gamemode },
                      { "verbose", _preferences.verbose },
                      { "debug", _preferences.debug } };
    }

    friend void from_json( const json& _json, preferences_t& _preferences ) {
        _json.at( "rollback" ).get_to( _preferences.rollback );
        _json.at( "delay" ).get_to( _preferences.delay );
        _json.at( "gamemode" ).get_to( _preferences.gamemode );
        _json.at( "verbose" ).get_to( _preferences.verbose );
        _json.at( "debug" ).get_to( _preferences.debug );
    }
} const g_preferencesDefault;

bool g_isVerbose = false;
bool g_isDebug = false;

int main( const int _argumentCount, const char* const* _argumentCVector ) {
    std::ofstream l_logFile;

    // Banana banana
    {
        const std::string l_folderName = FOLDER_NAME;
        const mode_t l_folderPermissions = FOLDER_PERMISSIONS;

        int l_returnCode = mkdir( l_folderName.c_str(), l_folderPermissions );

        const std::string l_logFileNamePostfix = LOG_FILE_NAME_POSTFIX;
        const std::string l_logFileName =
            fmt::format( "v{}.{}.{}_{}", g_versionMajor, g_versionMinor,
                         g_versionPatch, l_logFileNamePostfix );

        l_logFile.open( fmt::format( "{}/{}.{}", l_folderName, l_logFileName,
                                     LOG_FILE_EXTENSION ) );
        icecream::ic.output( l_logFile );

        IC0();
        std::vector< std::string > l_argumentVector(
            _argumentCVector, _argumentCVector + _argumentCount );
        IC( _argumentCount, l_argumentVector );

        std::bitset< 8 > l_folderPermissionsBinary( l_folderPermissions );
        IC( l_folderName, l_folderPermissionsBinary, l_returnCode,
            l_logFileName );
    }

    // Parse options
    {
        cxxopts::Options options( std::string( PROGRAM_NAME ),
                                  "Gameplay enhances for MBAACC" );

        options.add_options(
            "", { { "o,online", "launch online" },
                  { "s,spectate", "Launch spectate" },
                  { "t,tournament", "Launch tournament" },
                  { "p,practice", "Launch practice" },
                  { "c,cpu", "Launch versus CPU" },
                  { "verbose", "Verbose output",
                    cxxopts::value< bool >()->default_value( "true" ) },
                  { "debug", "Enable debugging",
                    cxxopts::value< bool >()->default_value( "false" ) },
                  { "h,help", "Display this information." } } );

        try {
            auto l_result = options.parse( _argumentCount, _argumentCVector );

            const auto _resultHas = [ l_result ]( std::string _key ) {
                IC( _key, l_result.count( _key ) );

                bool l_returnValue = false;

                if ( l_result.count( _key ) ) {
                    l_returnValue = true;
                }

                return ( l_returnValue );
            };

            // Help
            {
                if ( _resultHas( "help" ) ) {
                    IC( options.help() );

                    fmt::print( "{}\n", options.help() );

                    return ( 0 );
                }
            }

            // Global flags
            {
                const auto _resultGetFlag = [ &_resultHas, l_result ](
                                                std::string _key,
                                                bool _defaultValue ) -> bool {
                    IC( _key, _defaultValue );

                    bool l_returnValue = _defaultValue;

                    if ( _resultHas( _key ) ) {
                        IC( l_result[ _key ].as< bool >() );

                        l_returnValue = l_result[ _key ].as< bool >();
                    }

                    return ( l_returnValue );
                };

                g_isVerbose = _resultGetFlag( "verbose", g_isVerbose );

                IC( g_isVerbose );

                g_isDebug = _resultGetFlag( "verbose", g_isDebug );

                IC( g_isDebug );
            }

            // Parse JSON files
            {
                json l_jsonPreferences;

                // Preferences file
                {
                    const std::string l_folderName = FOLDER_NAME;
                    const std::string l_preferencesFileNamePostfix =
                        PREFERENCES_FILE_NAME_POSTFIX;
                    const std::string l_preferencesFileName =
                        fmt::format( "v{}_{}", g_versionMajor,
                                     l_preferencesFileNamePostfix );
                    const std::string l_preferencesFileExtension =
                        PREFERENCES_FILE_EXTENSION;
                    const std::string l_preferencesFilePath = fmt::format(
                        "{}/{}.{}", l_folderName, l_preferencesFileName,
                        l_preferencesFileExtension );

                    IC( l_preferencesFilePath );

                    // Parse preferences file
                    {
                        IC( l_jsonPreferences.empty() );

                        // Parse preferences file
                        {
                            std::ifstream l_jsonFileIn( l_preferencesFilePath );

                            // Is preferences file found
                            IC( l_jsonFileIn.good() );

                            if ( l_jsonFileIn.good() ) {
                                l_jsonPreferences = json::parse( l_jsonFileIn );

                            } else {
                                l_jsonPreferences = g_preferencesDefault;
                            }
                        }

                        // Set launch gamemode
                        {
                            const auto _trySetGamemode =
                                [ &_resultHas, l_result, &l_jsonPreferences ](
                                    std::string _gamemodeName,
                                    gamemode_t _gamemode ) {
                                    IC( _gamemodeName, _gamemode );

                                    if ( _resultHas( _gamemodeName ) ) {
                                        IC( l_jsonPreferences.dump( 4 ) );

                                        l_jsonPreferences[ "gamemode" ] =
                                            _gamemode;
                                    }
                                };

                            _trySetGamemode( "online", ONLINE );
                            _trySetGamemode( "spectate", SPECTATE );
                            _trySetGamemode( "tournament", TOURNAMENT );
                            _trySetGamemode( "practice", PRACTICE );
                            _trySetGamemode( "cpu", CPU );
                        }

                        IC( l_jsonPreferences.dump( 4 ) );
                    }

                    // Overwrite preferences file
                    {
                        std::ofstream l_jsonFileOut;

                        IC( l_preferencesFilePath );

                        l_jsonFileOut.open( l_preferencesFilePath );

                        if ( l_jsonFileOut.good() ) {
                            l_jsonFileOut << std::setw( 4 ) << l_jsonPreferences
                                          << std::endl;

                        } else {
                            try {
                                l_jsonFileOut.exceptions(
                                    l_jsonFileOut.failbit );

                            } catch ( const std::ios_base::failure&
                                          _jsonFileOutFailure ) {
                                const std::error_code l_errorCode =
                                    _jsonFileOutFailure.code();
                                const std::string l_what =
                                    _jsonFileOutFailure.what();

                                IC( l_errorCode.value(), l_what );
                                fmt::print( stderr, "{}: {}\n",
                                            l_errorCode.value(), l_what );
                            }
                        }
                    }
                }

                // Use preferences
                {}
            }

        } catch ( const cxxopts::exceptions::missing_argument&
                      _parserMissingArgument ) {
            const std::string l_what = _parserMissingArgument.what();

            IC( l_what );
            fmt::print( stderr, "{}\n", l_what );
        }
    }

    return ( 0 );
}
