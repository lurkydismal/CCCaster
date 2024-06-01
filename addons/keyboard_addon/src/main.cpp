#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "nlohmann/json.hpp"
// #include <fmt/core.h>
// #include <icecream/icecream.hpp>

#include <fstream>

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
                    MessageBoxA( 0, "50", "test", 0 );
                    std::ofstream l_jsonFileIn( "controls.json" );
                    MessageBoxA( 0, "70", "test", 0 );
                }
            }

            break;
        }
    }

    return ( l_returnValue );
}
