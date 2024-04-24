#include "addon_callbacks.hpp"

#include "_useCallback.hpp"
#include "d3d9.h"
#include "d3dx9.h"

HWND g_hFocusWindow = NULL;
useCallbackFunction_t g_useCallback = NULL;

extern "C" {

uint16_t __declspec( dllexport )
    IDirect3D9Ex$CreateDevice( void** _callbackArguments ) {
    MessageBoxA( 0, "dll", "test", 0 );
    //  HWND _hFocusWindow = ( HWND )_callbackArguments[ 2 ];
    //  D3DPRESENT_PARAMETERS* _pPresentationParameters =
    //      ( D3DPRESENT_PARAMETERS* )_callbackArguments[ 4 ];

    //  g_hFocusWindow =
    //      _hFocusWindow ? _hFocusWindow :
    //      _pPresentationParameters->hDeviceWindow;

    //  HMODULE l_statesDll = GetModuleHandleA( "states.dll" );

    //  if ( !l_statesDll ) {
    //      exit( 1 );
    //  }

    //  g_useCallback =
    //      ( useCallbackFunction_t )GetProcAddress( l_statesDll, "useCallback"
    //      );

    //  if ( !g_useCallback ) {
    //      exit( 1 );
    //  }

    return ( 0 );
}
}
