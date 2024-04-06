#include "addon_callbacks.hpp"

#include <vector>

#include "_useCallback.hpp"
#include "d3d9.h"
#include "d3dx9.h"
#include "imgui.h"
#include "imgui_impl_dx9.h"
#include "imgui_impl_win32.h"
#include "imgui_stdlib.h"

#pragma comment( lib, "user32.lib" )
#pragma comment( lib, "gdi32.lib" )
#pragma comment( lib, "dwmapi.lib" )

uint32_t g_activeFlags = 0;
useCallbackFunction_t g_useCallback = NULL;
bool g_isDraw = false;
extern bool g_imGuiInitialized;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler( HWND hWnd,
                                                              UINT msg,
                                                              WPARAM wParam,
                                                              LPARAM lParam );

extern "C" uint16_t __declspec( dllexport )
    IDirect3D9Ex$CreateDevice( void** _callbackArguments ) {
    HMODULE l_statesDll = GetModuleHandleA( "states.dll" );

    if ( !l_statesDll ) {
        exit( 1 );
    }

    g_useCallback =
        ( useCallbackFunction_t )GetProcAddress( l_statesDll, "useCallback" );

    if ( !g_useCallback ) {
        exit( 1 );
    }

    HWND _hFocusWindow = ( HWND )_callbackArguments[ 2 ];
    IDirect3DDevice9** _ppReturnedDeviceInterface =
        ( IDirect3DDevice9** )_callbackArguments[ 5 ];

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init( _hFocusWindow );
    ImGui_ImplDX9_Init( *_ppReturnedDeviceInterface );

    g_imGuiInitialized = true;

    return ( 0 );
}

extern "C" uint16_t __declspec( dllexport )
    IDirect3DDevice9Ex$EndScene( void** _callbackArguments ) {
    if ( ( g_activeFlags & SHOW_OVERLAY ) && ( g_isDraw ) ) {
        uint16_t l_result =
            _useCallback( "overlay$beforeDraw$ImGui", 1, &ImGui::Text );

        ImGui::End();

        ImGui::EndFrame();

        // Rendering
        ImGui::Render();
        ImGui_ImplDX9_RenderDrawData( ImGui::GetDrawData() );

        ImGuiIO& l_io = ImGui::GetIO();

        // Update and Render additional Platform Windows
        if ( l_io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable ) {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }

        g_isDraw = false;
    }

    return ( 0 );
}

extern "C" uint16_t __declspec( dllexport )
    IDirect3DDevice9Ex$PreReset( void** _callbackArguments ) {
    MessageBoxA( NULL,
                 "IDirect3DDevice9Ex$PreReset", // box text
                 "name",                        // box name
                 0 );

    ImGui_ImplDX9_InvalidateDeviceObjects();

    return ( 0 );
}

extern "C" uint16_t __declspec( dllexport )
    IDirect3DDevice9Ex$PostReset( void** _callbackArguments ) {
    MessageBoxA( NULL,
                 "IDirect3DDevice9Ex$PostReset", // box text
                 "name",                         // box name
                 0 );

    ImGui_ImplDX9_CreateDeviceObjects();

    return ( 0 );
}

extern "C" uint16_t __declspec( dllexport )
    CustomWindowProcedure( void** _callbackArguments ) {
    HWND _windowHandle = ( HWND )_callbackArguments[ 0 ];
    UINT _message = ( UINT )_callbackArguments[ 1 ];
    WPARAM _wideAdditionalMessage = ( WPARAM )_callbackArguments[ 2 ];
    LPARAM _lAdditionalMessage = ( LPARAM )_callbackArguments[ 3 ];
    // int idx = ( int )_callbackArguments[ 4 ];

    return ( ImGui_ImplWin32_WndProcHandler( _windowHandle, _message,
                                             _wideAdditionalMessage,
                                             _lAdditionalMessage ) );
}

extern "C" uint16_t __declspec( dllexport )
    extraDrawCallback( void** _callbackArguments ) {
    if ( !g_isDraw ) {
        // Start the Dear ImGui frame
        ImGui_ImplDX9_NewFrame();
        ImGui_ImplWin32_NewFrame();

        ImGui::NewFrame();

        ImGui::Begin( "Another Window" );

        g_isDraw = true;
    }

    return ( 0 );
}

extern "C" uint16_t __declspec( dllexport ) overlay$Toggle( void ) {
    uint16_t l_returnValue = 0;

    g_activeFlags ^= SHOW_OVERLAY;

    return ( g_activeFlags & SHOW_OVERLAY );
}
