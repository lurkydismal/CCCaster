#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <stdio.h>

#include <sstream>

#include "controls_parse.hpp"
#include "nlohmann/json.hpp"

json g_jsonControlsKeyboard;

BOOL WINAPI DllMain( HMODULE _moduleHandle, DWORD _callReason, LPVOID _ ) {
    bool l_returnValue = true;

    switch ( _callReason ) {
        case DLL_PROCESS_ATTACH: {
            // Parse JSON files
            {
                // Controls file
                {
                    const std::string l_controlsConfigFileName =
                        ( std::string( CONTROLS_PREFERENCES_FILE_NAME ) +
                          std::string( "." ) +
                          std::string( CONTROLS_PREFERENCES_FILE_EXTENSION ) );
                    const std::string l_controlsConfigBackupFileName =
                        ( std::string( CONTROLS_PREFERENCES_FILE_NAME ) +
                          std::string( BACKUP_ADDITION_TO_FILE_NAME ) +
                          std::string( "." ) +
                          std::string( CONTROLS_PREFERENCES_FILE_EXTENSION ) );

                    // Parse controls preferences file
                    try {
                        char* buffer =
                            readFile( l_controlsConfigFileName.c_str() );

                        std::string t;
                        if ( buffer ) {
                            t = std::string( buffer );
                            free( buffer );

                        } else {
                            buffer = readFile(
                                l_controlsConfigBackupFileName.c_str() );

                            if ( buffer ) {
                                t = std::string( buffer );
                                free( buffer );

                                writeFile( l_controlsConfigFileName.c_str(),
                                           t.c_str() );
                            }
                        }

                        g_jsonControlsKeyboard =
                            json::parse( t ).at( "keyboard" );

                    } catch ( const json::parse_error& e ) {
                        g_jsonControlsKeyboard = {
                            { "38", "8" },
                            { "39", "6" },
                            { "40", "2" },
                            { "37", "4" },
                            { "90", "A" },
                            { "88", "B" },
                            { "67", "C" },
                            { "86", "D" },
                            { "68", "E" },
                            { "83", "AB" },
                            { "221", "FN1" },
                            { "82", "FN2" },
                            { "84", "START" },
                            { "115", "ToggleOverlay_KeyConfig_Native" },
                        };

                        json l_jsonControls = {
                            { "keyboard", g_jsonControlsKeyboard },
                        };

                        std::string buffer =
                            l_jsonControls.dump( 4 ) + std::string( "\n" );

                        writeFile( l_controlsConfigBackupFileName.c_str(),
                                   buffer.c_str() );
                        writeFile( l_controlsConfigFileName.c_str(),
                                   buffer.c_str() );
                    }
                }
            }

            break;
        }
    }

    return ( l_returnValue );
}
