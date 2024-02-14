#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <nlohmann/json.hpp>
#include <fmt/core.h>
#include <icecream.hpp>
#include <fstream>

#define FOLDER_PERMISSIONS S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH

#define CONTROLS_PREFERENCES_FILE_NAME "controls"
#define CONTROLS_PREFERENCES_FILE_EXTENSION "json"

using json = nlohmann::json;

BOOL WINAPI DllMain( HMODULE _moduleHandle, DWORD _callReason, LPVOID _ ) {
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
                    const std::string l_controlsPreferencesFilePath = fmt::format(
                        "{}.{}", l_controlsPreferencesFileName,
                        l_controlsPreferencesFileExtension );

                    // IC( l_controlsPreferencesFilePath );

                    // Parse controls preferences file
                    {
                        // IC( l_jsonPreferences.empty() );

                        // Parse controls preferences file
                        {
                            std::ifstream l_jsonFileIn( l_controlsPreferencesFilePath );

                            // Is controls preferences file found
                            // IC( l_jsonFileIn.good() );

                            if ( l_jsonFileIn.good() ) {
                                l_jsonPreferences = json::parse( l_jsonFileIn );

                            } else {
                                const std::string l_controlsPreferencesBackupFilePath = fmt::format(
                                    "{}_backup.{}", l_controlsPreferencesFileName,
                                    l_controlsPreferencesFileExtension );

                                std::ifstream l_jsonBackupFileIn( l_controlsPreferencesBackupFilePath );

                                l_jsonPreferences = json::parse( l_jsonBackupFileIn );
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
                                fmt::print( stderr, "{}: {}\n",
                                            l_errorCode.value(), l_what );
                            }
                        }
                    }
                }

                // Use controls preferences
                {}
            }

            break;
        }
    }

    return ( true );
}
