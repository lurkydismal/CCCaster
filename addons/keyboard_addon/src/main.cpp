#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <fstream>
#include <sstream>

#include "nlohmann/json.hpp"
// #include <fmt/core.h>
// #include <icecream/icecream.hpp>

#include "controls_parse.hpp"

#define CONTROLS_PREFERENCES_FILE_NAME "controls"
#define CONTROLS_PREFERENCES_FILE_EXTENSION "json"
#define BACKUP_ADDITION_TO_FILE_NAME "_backup"

json g_jsonControlsKeyboard;

BOOL WINAPI DllMain( HMODULE _moduleHandle, DWORD _callReason, LPVOID _ ) {
    bool l_returnValue = true;

    switch ( _callReason ) {
        case DLL_PROCESS_ATTACH: {
            // Parse JSON files
            {
                // Preferences file
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
                        // parsing input with a syntax error
                        MessageBoxA( 0, "1", "test", 0 );

                        char* buffer = 0;
                        long length;
                        FILE* f =
                            fopen( l_controlsConfigFileName.c_str(), "rb" );

                        if ( f ) {
                            fseek( f, 0, SEEK_END );
                            length = ftell( f );
                            fseek( f, 0, SEEK_SET );
                            buffer = ( char* )malloc( length );
                            if ( buffer ) {
                                fread( buffer, 1, length, f );
                            }
                            fclose( f );
                        }

                        std::string t;
                        if ( buffer ) {
                            t = std::string( buffer );
                            free( buffer );

                            MessageBoxA( 0, "2", "test", 0 );
                            MessageBoxA( 0, t.c_str(), "test", 0 );

                        } else {
                            buffer = 0;
                            f = fopen( l_controlsConfigBackupFileName.c_str(),
                                       "rb" );

                            if ( f ) {
                                fseek( f, 0, SEEK_END );
                                length = ftell( f );
                                fseek( f, 0, SEEK_SET );
                                buffer = ( char* )malloc( length );
                                if ( buffer ) {
                                    fread( buffer, 1, length, f );
                                }
                                fclose( f );
                            }

                            if ( buffer ) {
                                t = std::string( buffer );
                                free( buffer );

                                MessageBoxA( 0, "3", "test", 0 );
                                MessageBoxA( 0, t.c_str(), "test", 0 );
                            }
                        }

                        MessageBoxA( 0, "6", "test", 0 );
                        MessageBoxA( 0, t.c_str(), "test", 0 );
                        g_jsonControlsKeyboard = json::parse( t );

                    } catch ( const json::parse_error& e ) {
                        // output exception information
                        MessageBoxA( 0, std::string( e.what() ).c_str(), "test",
                                     0 );

                        MessageBoxA( 0, "4", "test", 0 );
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

                        std::string t = g_jsonControlsKeyboard.dump( 4 ) +
                                        std::string( "\n" );

                        const char* buffer = t.c_str();
                        FILE* f =
                            fopen( l_controlsConfigFileName.c_str(), "wb" );

                        if ( f ) {
                            fprintf( f, "%s\n", buffer );
                        }

                        MessageBoxA( 0, "5", "test", 0 );
                    }
                }
            }

            break;
        }
    }

    return ( l_returnValue );
}
