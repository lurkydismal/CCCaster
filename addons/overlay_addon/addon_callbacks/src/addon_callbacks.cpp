#include "addon_callbacks.hpp"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <vector>

#include "_useCallback.h"

#pragma comment( lib, "user32.lib" )
#pragma comment( lib, "gdi32.lib" )
#pragma comment( lib, "dwmapi.lib" )

uint32_t g_activeFlags = 0;
useCallbackFunction_t g_useCallback = NULL;

extern "C" uint16_t __declspec( dllexport ) IDirect3D9Ex$CreateDevice(
    void** _callbackArguments ) {
    HMODULE l_statesDll = GetModuleHandleA( "states.dll" );

    if ( !l_statesDll ) {
        exit( 1 );
    }

    g_useCallback =
        ( useCallbackFunction_t )GetProcAddress( l_statesDll, "useCallback" );

    if ( !g_useCallback ) {
        exit( 1 );
    }

    return ( 0 );
}

extern "C" uint16_t __declspec( dllexport ) overlay$getState(
    void** _callbackArguments ) {
    uint16_t l_returnValue = 0;

    return ( g_activeFlags & SHOW_OVERLAY );
}

extern "C" uint16_t __declspec( dllexport ) overlay$toggle(
    void** _callbackArguments ) {
    uint16_t l_returnValue = 0;

    g_activeFlags ^= SHOW_OVERLAY;

    return ( g_activeFlags & SHOW_OVERLAY );
}
