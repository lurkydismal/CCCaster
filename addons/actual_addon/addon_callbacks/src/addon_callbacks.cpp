#include "addon_callbacks.hpp"

#include "_useCallback.hpp"
#include "d3d9.h"
#include "d3dx9.h"

HWND g_hFocusWindow = NULL;
useCallbackFunction_t g_useCallback = NULL;

extern "C" uint16_t __declspec( dllexport ) IDirect3D9Ex$CreateDevice(
    void** _callbackArguments ) {
    HWND _hFocusWindow = ( HWND )_callbackArguments[ 2 ];
    MessageBoxA( 0, "1", "test", 0 );
    D3DPRESENT_PARAMETERS* _pPresentationParameters =
        ( D3DPRESENT_PARAMETERS* )_callbackArguments[ 4 ];
    MessageBoxA( 0, "2", "test", 0 );

    g_hFocusWindow =
        _hFocusWindow ? _hFocusWindow : _pPresentationParameters->hDeviceWindow;
    MessageBoxA( 0, "3", "test", 0 );

    HMODULE l_statesDll = GetModuleHandleA( "states.dll" );
    MessageBoxA( 0, "4", "test", 0 );

    if ( !l_statesDll ) {
        MessageBoxA( 0, "5", "test", 0 );
        exit( 1 );
    }

    g_useCallback =
        ( useCallbackFunction_t )GetProcAddress( l_statesDll, "useCallback" );

    MessageBoxA( 0, "6", "test", 0 );
    if ( !g_useCallback ) {
        MessageBoxA( 0, "7", "test", 0 );
        exit( 1 );
    }

    return ( 0 );
}
