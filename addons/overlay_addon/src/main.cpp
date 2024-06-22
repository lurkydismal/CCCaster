#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// #include <fmt/core.h>
// #include <icecream/icecream.hpp>

#include "addon_callbacks.hpp"
#include "imgui.h"
#include "imgui_impl_dx9.h"
#include "imgui_impl_win32.h"
#include "imgui_stdlib.h"

#pragma comment( lib, "user32.lib" )

bool g_imGuiInitialized = false;

BOOL WINAPI DllMain( HMODULE _moduleHandle, DWORD _callReason, LPVOID _ ) {
    bool l_returnValue = true;

    switch ( _callReason ) {
        case DLL_PROCESS_ATTACH: {
            if ( !g_imGuiInitialized ) {
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
                                                       // l_io.ConfigFlags |=
                //     ImGuiConfigFlags_DockingEnable; // Enable Docking
                // l_io.ConfigFlags |=
                //     ImGuiConfigFlags_ViewportsEnable; // Enable
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
