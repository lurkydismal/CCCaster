#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "nlohmann/json.hpp"
// #include <fmt/core.h>
// #include <icecream/icecream.hpp>
#include <fstream>

#include "button_input.h"
#include "button_mappings.hpp"
#include "direction_input.h"
#include "direction_mappings.hpp"

#define CONTROLS_PREFERENCES_FILE_NAME "controls"
#define CONTROLS_PREFERENCES_FILE_EXTENSION "json"

using json = nlohmann::json;

BOOL WINAPI DllMain( HMODULE _moduleHandle, DWORD _callReason, LPVOID _ ) {
    bool l_returnValue = true;

    switch ( _callReason ) {
        case DLL_PROCESS_ATTACH: {
            // Parse JSON files
            {
                json l_jsonPreferences;

                // Preferences file
                {
                    const std::string l_controlsPreferencesFileName =
                        CONTROLS_PREFERENCES_FILE_NAME;
                    const std::string l_controlsPreferencesFileExtension =
                        CONTROLS_PREFERENCES_FILE_EXTENSION;
                    // const std::string l_controlsPreferencesFilePath =
                    // fmt::format(
                    //     "{}.{}", l_controlsPreferencesFileName,
                    //     l_controlsPreferencesFileExtension );

                    const std::string l_controlsPreferencesFilePath =
                        l_controlsPreferencesFileName + "." +
                        l_controlsPreferencesFileExtension;

                    // IC( l_controlsPreferencesFilePath );

                    // Parse controls preferences file
                    {
                        ;
                        // IC( l_jsonPreferences.empty() );

                        // Parse controls preferences file
                        {
                            ;
                            std::ifstream l_jsonFileIn(
                                l_controlsPreferencesFilePath );

                            // Is controls preferences file found
                            // IC( l_jsonFileIn.good() );

                            if ( l_jsonFileIn.good() ) {
                                l_jsonPreferences = json::parse( l_jsonFileIn );

                            } else {
                                // const std::string
                                // l_controlsPreferencesBackupFilePath =
                                // fmt::format(
                                //     "{}_backup.{}",
                                //     l_controlsPreferencesFileName,
                                //     l_controlsPreferencesFileExtension );

                                const std::string
                                    l_controlsPreferencesBackupFilePath =
                                        l_controlsPreferencesFileName +
                                        "_backup." +
                                        l_controlsPreferencesFileExtension;

                                std::ifstream l_jsonBackupFileIn(
                                    l_controlsPreferencesBackupFilePath );

                                l_jsonPreferences =
                                    json::parse( l_jsonBackupFileIn );
                            }
                        }

                        // IC( l_jsonPreferences.dump( 4 ) );
                    }

                    // Overwrite controls preferences file
                    {
                        std::ofstream l_jsonFileOut;

                        // IC( l_controlsPreferencesFilePath );

                        l_jsonFileOut.open( l_controlsPreferencesFilePath );

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

                                // IC( l_errorCode.value(), l_what );
                                // fmt::print( stderr, "{}: {}\n",
                                //             l_errorCode.value(), l_what );
                                printf( "%d: %s\n", l_errorCode.value(),
                                        l_what.c_str() );
                            }
                        }
                    }
                }

                // Use controls preferences
                {
                    const json* l_jsonKeyboardPreferences =
                        &l_jsonPreferences.at( "keyboard" );

                    std::map< direction_t, char > l_directionMappings = {
                        { UP, ( *l_jsonKeyboardPreferences )
                                  .at( "8" )
                                  .get< char >() },
                        { RIGHT, ( *l_jsonKeyboardPreferences )
                                     .at( "6" )
                                     .get< char >() },
                        { DOWN, ( *l_jsonKeyboardPreferences )
                                    .at( "2" )
                                    .get< char >() },
                        { LEFT, ( *l_jsonKeyboardPreferences )
                                    .at( "4" )
                                    .get< char >() } };

                    l_returnValue = setDirectionMappings( l_directionMappings );

                    if ( l_returnValue ) {
                        std::vector< std::pair< char, button_t > >
                            l_buttonMappings = {
                                { ( *l_jsonKeyboardPreferences )
                                      .at( "A" )
                                      .get< char >(),
                                  static_cast< button_t >( A | CONFIRM ) },
                                { ( *l_jsonKeyboardPreferences )
                                      .at( "B" )
                                      .get< char >(),
                                  static_cast< button_t >( B | CANCEL ) },
                                { ( *l_jsonKeyboardPreferences )
                                      .at( "C" )
                                      .get< char >(),
                                  C },
                                { ( *l_jsonKeyboardPreferences )
                                      .at( "D" )
                                      .get< char >(),
                                  D },
                                { ( *l_jsonKeyboardPreferences )
                                      .at( "E" )
                                      .get< char >(),
                                  E },
                                { ( *l_jsonKeyboardPreferences )
                                      .at( "AB" )
                                      .get< char >(),
                                  AB },
                                { ( *l_jsonKeyboardPreferences )
                                      .at( "FN1" )
                                      .get< char >(),
                                  FN1 },
                                { ( *l_jsonKeyboardPreferences )
                                      .at( "FN2" )
                                      .get< char >(),
                                  FN2 },
                                { ( *l_jsonKeyboardPreferences )
                                      .at( "START" )
                                      .get< char >(),
                                  START } };

                        l_returnValue = setButtonMappings( l_buttonMappings );
                    }
                }
            }

            break;
        }
    }

    return ( l_returnValue );
}
