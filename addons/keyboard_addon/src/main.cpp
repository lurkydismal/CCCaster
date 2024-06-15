#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "nlohmann/json.hpp"
// #include <fmt/core.h>
// #include <icecream/icecream.hpp>

#include "button_input.h"
#include "controls_parse.hpp"
#include "direction_input.h"
#include "json_file.hpp"

#define CONTROLS_PREFERENCES_FILE_NAME "controls"
#define CONTROLS_PREFERENCES_FILE_EXTENSION "json"

using json = nlohmann::json;

json g_jsonControlsKeyboard;

BOOL WINAPI DllMain( HMODULE _moduleHandle, DWORD _callReason, LPVOID _ ) {
    bool l_returnValue = true;

    switch ( _callReason ) {
        case DLL_PROCESS_ATTACH: {
            // Parse JSON files
            {
                // Preferences file
                {
                    // Parse controls preferences file
                }

                // Use controls preferences
                {
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
                        { "114", "ToggleOverlay_Netplay" },
                        { "113", "ToggleOverlay_KeyConfig" },
                    };
                }
            }

            break;
        }
    }

    return ( l_returnValue );
}
