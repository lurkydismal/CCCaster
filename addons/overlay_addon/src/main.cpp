#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "nlohmann/json.hpp"
// #include <fmt/core.h>
// #include <icecream/icecream.hpp>
#include <fstream>

#include "addon_callbacks.hpp"
#include "button_mappings.hpp"
#include "imgui.h"
#include "imgui_impl_dx9.h"
#include "imgui_impl_win32.h"
#include "imgui_stdlib.h"

#pragma comment( lib, "user32.lib" )

#define CONTROLS_PREFERENCES_FILE_NAME "controls"
#define CONTROLS_PREFERENCES_FILE_EXTENSION "json"

using json = nlohmann::json;

bool g_imGuiInitialized = false;

BOOL WINAPI DllMain( HMODULE _moduleHandle, DWORD _callReason, LPVOID _ ) {
    bool l_returnValue = true;

    switch ( _callReason ) {
        case DLL_PROCESS_ATTACH: {
            if ( !g_imGuiInitialized ) {
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
                                    l_jsonPreferences =
                                        json::parse( l_jsonFileIn );

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
                                l_jsonFileOut << std::setw( 4 )
                                              << l_jsonPreferences << std::endl;

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
                                    //             l_errorCode.value(), l_what
                                    //             );
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

                        std::map< std::string, char > l_buttonMappings = {
                            { "ToggleOverlay", ( *l_jsonKeyboardPreferences )
                                                   .at( "ToggleOverlay" )
                                                   .get< char >() } };

                        l_returnValue = setButtonMappings( l_buttonMappings );
                    }
                }

                // Setup Dear ImGui context
                IMGUI_CHECKVERSION();

                ImGui::CreateContext();

                ImGuiIO& l_io = ImGui::GetIO();
                l_io.ConfigFlags |=
                    ImGuiConfigFlags_NavEnableKeyboard; // Enable
                                                        // Keyboard
                                                        // Controls
                l_io.ConfigFlags |=
                    ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad
                                                       // Controls
                l_io.ConfigFlags |=
                    ImGuiConfigFlags_DockingEnable; // Enable Docking
                l_io.ConfigFlags |=
                    ImGuiConfigFlags_ViewportsEnable; // Enable
                                                      // Multi-Viewport
                                                      // / Platform
                                                      // Windows

                // Setup Dear ImGui style
                ImGui::StyleColorsDark();

                // When viewports are enabled we tweak
                // WindowRounding/WindowBg so platform windows can look
                // identical to regular ones.
                ImGuiStyle& l_imGUIStyle = ImGui::GetStyle();

                if ( l_io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable ) {
                    l_imGUIStyle.WindowRounding = 0.0f;
                    l_imGUIStyle.Colors[ ImGuiCol_WindowBg ].w = 1.0f;
                }
            }

            break;
        }

        case DLL_PROCESS_DETACH: {
            if ( g_imGuiInitialized ) {
                ImGui_ImplDX9_Shutdown();
                ImGui_ImplWin32_Shutdown();
                ImGui::DestroyContext();
            }

            break;
        }
    }

    return ( l_returnValue );
}
