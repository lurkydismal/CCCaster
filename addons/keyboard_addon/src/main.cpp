#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "nlohmann/json.hpp"
// #include <fmt/core.h>
// #include <icecream/icecream.hpp>

#include "button_input.h"
#include "button_mappings.hpp"
#include "controls_parse.hpp"
#include "direction_input.h"
#include "direction_mappings.hpp"
#include "hotkey_mappings.hpp"
#include "json_file.hpp"

#define CONTROLS_PREFERENCES_FILE_NAME "controls"
#define CONTROLS_PREFERENCES_FILE_EXTENSION "json"

using json = nlohmann::json;

JSONFile g_jsonControlsKeyboard;

BOOL WINAPI DllMain( HMODULE _moduleHandle, DWORD _callReason, LPVOID _ ) {
    bool l_returnValue = true;

    switch ( _callReason ) {
        case DLL_PROCESS_ATTACH: {
            // Parse JSON files
            {
                // Preferences file
                {
                    // Parse controls preferences file
                    g_jsonControlsKeyboard =
                        JSONFile( CONTROLS_PREFERENCES_FILE_NAME,
                                  CONTROLS_PREFERENCES_FILE_EXTENSION );

                    g_jsonControlsKeyboard.overwrite();
                }

                // Use controls preferences
                {
                    json l_jsonPreferencesKeyboard =
                        g_jsonControlsKeyboard.get().at( "keyboard" );

                    std::map< direction_t, char > l_directionMappings = {
                        { UP, ( l_jsonPreferencesKeyboard )
                                  .at( "8" )
                                  .get< char >() },
                        { RIGHT, ( l_jsonPreferencesKeyboard )
                                     .at( "6" )
                                     .get< char >() },
                        { DOWN, ( l_jsonPreferencesKeyboard )
                                    .at( "2" )
                                    .get< char >() },
                        { LEFT, ( l_jsonPreferencesKeyboard )
                                    .at( "4" )
                                    .get< char >() } };

                    l_returnValue = setDirectionMappings( l_directionMappings );

                    if ( l_returnValue ) {
                        std::vector< std::pair< char, button_t > >
                            l_buttonMappings = {
                                { ( l_jsonPreferencesKeyboard )
                                      .at( "A" )
                                      .get< char >(),
                                  static_cast< button_t >( A | CONFIRM ) },
                                { ( l_jsonPreferencesKeyboard )
                                      .at( "B" )
                                      .get< char >(),
                                  static_cast< button_t >( B | CANCEL ) },
                                { ( l_jsonPreferencesKeyboard )
                                      .at( "C" )
                                      .get< char >(),
                                  C },
                                { ( l_jsonPreferencesKeyboard )
                                      .at( "D" )
                                      .get< char >(),
                                  D },
                                { ( l_jsonPreferencesKeyboard )
                                      .at( "E" )
                                      .get< char >(),
                                  E },
                                { ( l_jsonPreferencesKeyboard )
                                      .at( "AB" )
                                      .get< char >(),
                                  AB },
                                { ( l_jsonPreferencesKeyboard )
                                      .at( "FN1" )
                                      .get< char >(),
                                  FN1 },
                                { ( l_jsonPreferencesKeyboard )
                                      .at( "FN2" )
                                      .get< char >(),
                                  FN2 },
                                { ( l_jsonPreferencesKeyboard )
                                      .at( "START" )
                                      .get< char >(),
                                  START } };

                        l_returnValue = setButtonMappings( l_buttonMappings );

                        if ( l_returnValue ) {
                            std::map< std::string, char > l_hotkeyMappings = {
                                { "ToggleOverlay_KeyConfig",
                                  ( l_jsonPreferencesKeyboard )
                                      .at( "ToggleOverlay_KeyConfig" )
                                      .get< char >() },
                                { "ToggleOverlay_KeyConfig_Native",
                                  ( l_jsonPreferencesKeyboard )
                                      .at( "ToggleOverlay_KeyConfig_Native" )
                                      .get< char >() } };

                            l_returnValue =
                                setHotkeyMappings( l_hotkeyMappings );
                        }
                    }
                }
            }

            break;
        }
    }

    return ( l_returnValue );
}
