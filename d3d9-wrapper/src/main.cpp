#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <stdint.h>

#include <string>

#include "_useCallback.h"
#include "d3d9.h"
#include "d3dx9.h"
#include "helpers.h"

#pragma comment( lib, "d3dx9.lib" )
#pragma comment( lib, "user32.lib" )

static size_t l_renderCallsPerFrame = 0;

Direct3DShaderValidatorCreate9Proc m_pDirect3DShaderValidatorCreate9;
PSGPErrorProc m_pPSGPError;
PSGPSampleTextureProc m_pPSGPSampleTexture;
D3DPERF_BeginEventProc m_pD3DPERF_BeginEvent;
D3DPERF_EndEventProc m_pD3DPERF_EndEvent;
D3DPERF_GetStatusProc m_pD3DPERF_GetStatus;
D3DPERF_QueryRepeatFrameProc m_pD3DPERF_QueryRepeatFrame;
D3DPERF_SetMarkerProc m_pD3DPERF_SetMarker;
D3DPERF_SetOptionsProc m_pD3DPERF_SetOptions;
D3DPERF_SetRegionProc m_pD3DPERF_SetRegion;
DebugSetLevelProc m_pDebugSetLevel;
DebugSetMuteProc m_pDebugSetMute;
Direct3D9EnableMaximizedWindowedModeShimProc
    m_pDirect3D9EnableMaximizedWindowedModeShim;
Direct3DCreate9Proc m_pDirect3DCreate9;
Direct3DCreate9ExProc m_pDirect3DCreate9Ex;

HWND g_hFocusWindow = NULL;
HMODULE g_hWrapperModule = NULL;

HMODULE g_statesDll = NULL;
HMODULE g_d3d9dll = NULL;
HMODULE g_justAnotherModloaderDll = NULL;

useCallbackFunction_t g_useCallback = NULL;

HRESULT m_IDirect3DDevice9Ex::Present( CONST RECT* pSourceRect,
                                       CONST RECT* pDestRect,
                                       HWND hDestWindowOverride,
                                       CONST RGNDATA* pDirtyRegion ) {
    l_renderCallsPerFrame = 0;

#if 0
    const char l_message[] =
        "m_IDirect3DDevice9Ex Present ( pSourceRect, pDestRect, "
        "hDestWindowOverride, pDirtyRegion )\n";

    _useCallback( "log$transaction$query", l_message );
    _useCallback( "log$transaction$commit" );
#endif

    uint16_t l_result =
        _useCallback( "IDirect3DDevice9Ex$Present", pSourceRect, pDestRect,
                      hDestWindowOverride, pDirtyRegion );

    if ( ( l_result != 0 ) && ( l_result != ENODATA ) ) {
        return ( E_FAIL );
    }

    return ( ProxyInterface->Present( pSourceRect, pDestRect,
                                      hDestWindowOverride, pDirtyRegion ) );
}

HRESULT m_IDirect3DDevice9Ex::PresentEx( THIS_ CONST RECT* pSourceRect,
                                         CONST RECT* pDestRect,
                                         HWND hDestWindowOverride,
                                         CONST RGNDATA* pDirtyRegion,
                                         DWORD dwFlags ) {
    const char l_message[] =
        "m_IDirect3DDevice9Ex PresentEx ( pSourceRect, pDestRect, "
        "hDestWindowOverride, pDirtyRegion, dwFlags )\n";

    _useCallback( "log$transaction$query", l_message );
    _useCallback( "log$transaction$commit" );

    uint16_t l_result =
        _useCallback( "IDirect3DDevice9Ex$PresentEx", pSourceRect, pDestRect,
                      hDestWindowOverride, pDirtyRegion, &dwFlags );

    if ( ( l_result != 0 ) && ( l_result != ENODATA ) ) {
        return ( E_FAIL );
    }

    return ( ProxyInterface->PresentEx(
        pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion, dwFlags ) );
}

HRESULT m_IDirect3DDevice9Ex::EndScene( void ) {
    l_renderCallsPerFrame++;

#if 0
    const char l_message[] = "m_IDirect3DDevice9Ex EndScene ()\n";

    _useCallback( "log$transaction$query", l_message );
    _useCallback( "log$transaction$commit" );
#endif

    uint16_t l_result = _useCallback( "IDirect3DDevice9Ex$EndScene" );

    if ( ( l_result != 0 ) && ( l_result != ENODATA ) ) {
        return ( E_FAIL );
    }

    return ( ProxyInterface->EndScene() );
}

HRESULT m_IDirect3D9Ex::CreateDevice(
    UINT Adapter,
    D3DDEVTYPE DeviceType,
    HWND hFocusWindow,
    DWORD BehaviorFlags,
    D3DPRESENT_PARAMETERS* pPresentationParameters,
    IDirect3DDevice9** ppReturnedDeviceInterface ) {
    g_hFocusWindow =
        hFocusWindow ? hFocusWindow : pPresentationParameters->hDeviceWindow;

    HRESULT hr = ProxyInterface->CreateDevice(
        Adapter, DeviceType, hFocusWindow, BehaviorFlags,
        pPresentationParameters, ppReturnedDeviceInterface );

    if ( SUCCEEDED( hr ) && ppReturnedDeviceInterface ) {
        *ppReturnedDeviceInterface = new m_IDirect3DDevice9Ex(
            ( IDirect3DDevice9Ex* )*ppReturnedDeviceInterface, this,
            IID_IDirect3DDevice9 );
    }

    const char l_message[] =
        "IDirect3D9Ex CreateDevice ( Adapter, DeviceType, hFocusWindow, "
        "BehaviorFlags, pPresentationParameters, ppReturnedDeviceInterface)\n";

    _useCallback( "log$transaction$query", l_message );
    _useCallback( "log$transaction$commit" );

    uint16_t l_result = _useCallback(
        "IDirect3D9Ex$CreateDevice", &Adapter, &DeviceType, &hFocusWindow,
        &BehaviorFlags, &pPresentationParameters, &ppReturnedDeviceInterface );

    if ( ( l_result != 0 ) && ( l_result != ENODATA ) ) {
        return ( E_FAIL );
    }

    return ( hr );
}

HRESULT m_IDirect3DDevice9Ex::Reset(
    D3DPRESENT_PARAMETERS* pPresentationParameters ) {
    const char l_message[] =
        "m_IDirect3DDevice9Ex Reset ( pPresentationParameters )\n";

    _useCallback( "log$transaction$query", l_message );
    _useCallback( "log$transaction$commit" );

    uint16_t l_result =
        _useCallback( "IDirect3DDevice9Ex$PreReset", pPresentationParameters );

    if ( ( l_result != 0 ) && ( l_result != ENODATA ) ) {
        return ( E_FAIL );
    }

    auto hRet = ProxyInterface->Reset( pPresentationParameters );

    l_result =
        _useCallback( "IDirect3DDevice9Ex$PostReset", pPresentationParameters );

    if ( ( l_result != 0 ) && ( l_result != ENODATA ) ) {
        return ( E_FAIL );
    }

    return ( hRet );
}

HRESULT m_IDirect3D9Ex::CreateDeviceEx(
    THIS_ UINT Adapter,
    D3DDEVTYPE DeviceType,
    HWND hFocusWindow,
    DWORD BehaviorFlags,
    D3DPRESENT_PARAMETERS* pPresentationParameters,
    D3DDISPLAYMODEEX* pFullscreenDisplayMode,
    IDirect3DDevice9Ex** ppReturnedDeviceInterface ) {
    g_hFocusWindow = ( hFocusWindow )
                         ? ( hFocusWindow )
                         : ( pPresentationParameters->hDeviceWindow );

    HRESULT hr = ProxyInterface->CreateDeviceEx(
        Adapter, DeviceType, hFocusWindow, BehaviorFlags,
        pPresentationParameters, pFullscreenDisplayMode,
        ppReturnedDeviceInterface );

    if ( SUCCEEDED( hr ) && ppReturnedDeviceInterface ) {
        *ppReturnedDeviceInterface = new m_IDirect3DDevice9Ex(
            *ppReturnedDeviceInterface, this, IID_IDirect3DDevice9Ex );
    }

    const char l_message[] =
        "IDirect3D9Ex CreateDeviceEx ( Adapter, DeviceType, hFocusWindow, "
        "BehaviorFlags, pPresentationParameters, pFullscreenDisplayMode, "
        "ppReturnedDeviceInterface )\n";

    _useCallback( "log$transaction$query", l_message );
    _useCallback( "log$transaction$commit" );

    uint16_t l_result =
        _useCallback( "IDirect3D9Ex$CreateDeviceEx", &Adapter, &DeviceType,
                      hFocusWindow, &BehaviorFlags, pPresentationParameters,
                      pFullscreenDisplayMode, ppReturnedDeviceInterface );

    if ( ( l_result != 0 ) && ( l_result != ENODATA ) ) {
        return ( E_FAIL );
    }

    return ( hr );
}

HRESULT m_IDirect3DDevice9Ex::ResetEx(
    THIS_ D3DPRESENT_PARAMETERS* pPresentationParameters,
    D3DDISPLAYMODEEX* pFullscreenDisplayMode ) {
    const char l_message[] =
        "m_IDirect3DDevice9Ex ResetEx ( pPresentationParameters, "
        "pFullscreenDisplayMode )\n";

    _useCallback( "log$transaction$query", l_message );
    _useCallback( "log$transaction$commit" );

    uint16_t l_result =
        _useCallback( "IDirect3DDevice9Ex$PreResetEx", pPresentationParameters,
                      pFullscreenDisplayMode );

    if ( ( l_result != 0 ) && ( l_result != ENODATA ) ) {
        return ( E_FAIL );
    }

    auto hRet = ProxyInterface->ResetEx( pPresentationParameters,
                                         pFullscreenDisplayMode );

    l_result = _useCallback( "IDirect3DDevice9Ex$PostResetEx",
                             pPresentationParameters, pFullscreenDisplayMode );

    if ( ( l_result != 0 ) && ( l_result != ENODATA ) ) {
        return ( E_FAIL );
    }

    return ( hRet );
}

static bool loadDll( const char* _DLLName, HMODULE* _moduleHandle ) {
    bool l_returnValue = true;
    char path[ MAX_PATH ];

    GetSystemDirectoryA( path, MAX_PATH );
    strcat( path, "/" );
    strcat( path, _DLLName );

    *_moduleHandle = LoadLibraryA( path );

    if ( !( *_moduleHandle ) ) {
        l_returnValue = false;
    }

    return ( l_returnValue );
}

extern "C" BOOL WINAPI DllMain( HMODULE hModule,
                                DWORD dwReason,
                                LPVOID lpReserved ) {
    switch ( dwReason ) {
        case DLL_PROCESS_ATTACH: {
            g_hWrapperModule = hModule;

            bool l_d3d9LoadResult = loadDll( "d3d9.dll", &g_d3d9dll );

            if ( l_d3d9LoadResult ) {
                if ( g_d3d9dll ) {
#define GET_ADDRESS( _functionName ) \
    m_p##_functionName =             \
        ( _functionName##Proc )GetProcAddress( g_d3d9dll, #_functionName );

                    GET_ADDRESS( Direct3DShaderValidatorCreate9 )
                    GET_ADDRESS( PSGPError )
                    GET_ADDRESS( PSGPSampleTexture )
                    GET_ADDRESS( D3DPERF_BeginEvent )
                    GET_ADDRESS( D3DPERF_EndEvent )
                    GET_ADDRESS( D3DPERF_GetStatus )
                    GET_ADDRESS( D3DPERF_QueryRepeatFrame )
                    GET_ADDRESS( D3DPERF_SetMarker )
                    GET_ADDRESS( D3DPERF_SetOptions )
                    GET_ADDRESS( D3DPERF_SetRegion )
                    GET_ADDRESS( DebugSetLevel )
                    GET_ADDRESS( DebugSetMute )
                    GET_ADDRESS( Direct3D9EnableMaximizedWindowedModeShim )
                    GET_ADDRESS( Direct3DCreate9 )
                    GET_ADDRESS( Direct3DCreate9Ex )

#undef Transtelecom
                }

                // Wake mode for OS >= Windows Vista
                uint32_t l_executionThreadStateFlags =
                    ( ES_CONTINUOUS | ES_DISPLAY_REQUIRED | ES_SYSTEM_REQUIRED |
                      ES_AWAYMODE_REQUIRED );

                if ( !SetThreadExecutionState( l_executionThreadStateFlags ) ) {
                    // Wake mode for OS < Windows Vista
                    l_executionThreadStateFlags ^= ES_AWAYMODE_REQUIRED;

                    SetThreadExecutionState( l_executionThreadStateFlags );
                }

                g_justAnotherModloaderDll =
                    LoadLibraryA( "just_another_modloader.dll" );

                if ( !g_justAnotherModloaderDll ) {
                    exit( 1 );
                }

                g_statesDll = GetModuleHandleA( "states.dll" );

                if ( !g_statesDll ) {
                    exit( 1 );
                }

                g_useCallback = ( useCallbackFunction_t )GetProcAddress(
                    g_statesDll, "useCallback" );

                if ( !g_useCallback ) {
                    exit( 1 );
                }

            } else {
                exit( 1 );
            }

            break;
        }
    }

    return ( true );
}

extern "C" HRESULT WINAPI Direct3DShaderValidatorCreate9( void ) {
#if 0
    const char l_message[] = "Direct3DShaderValidatorCreate9 ()\n";

    _useCallback( "log$transaction$query", l_message );
    _useCallback( "log$transaction$commit" );
#endif

    if ( !m_pDirect3DShaderValidatorCreate9 ) {
        return ( E_FAIL );
    }

    return ( m_pDirect3DShaderValidatorCreate9() );
}

extern "C" HRESULT WINAPI PSGPError( void ) {
    const char l_message[] = "PSGPError ()\n";

    _useCallback( "log$transaction$query", l_message );
    _useCallback( "log$transaction$commit" );

    if ( !m_pPSGPError ) {
        return ( E_FAIL );
    }

    return ( m_pPSGPError() );
}

extern "C" HRESULT WINAPI PSGPSampleTexture( void ) {
    const char l_message[] = "PSGPSampleTexture ()\n";

    _useCallback( "log$transaction$query", l_message );
    _useCallback( "log$transaction$commit" );

    if ( !m_pPSGPSampleTexture ) {
        return ( E_FAIL );
    }

    return ( m_pPSGPSampleTexture() );
}

extern "C" int WINAPI D3DPERF_BeginEvent( D3DCOLOR col, LPCWSTR wszName ) {
    const char l_message[] = "D3DPERF_BeginEvent ( col, wszName )\n";

    _useCallback( "log$transaction$query", l_message );
    _useCallback( "log$transaction$commit" );

    if ( !m_pD3DPERF_BeginEvent ) {
        return ( 0 );
    }

    return ( m_pD3DPERF_BeginEvent( col, wszName ) );
}

extern "C" int WINAPI D3DPERF_EndEvent( void ) {
    const char l_message[] = "D3DPERF_EndEvent ()\n";

    _useCallback( "log$transaction$query", l_message );
    _useCallback( "log$transaction$commit" );

    if ( !m_pD3DPERF_EndEvent ) {
        return ( 0 );
    }

    return ( m_pD3DPERF_EndEvent() );
}

extern "C" DWORD WINAPI D3DPERF_GetStatus( void ) {
    const char l_message[] = "D3DPERF_GetStatus ()\n";

    _useCallback( "log$transaction$query", l_message );
    _useCallback( "log$transaction$commit" );

    if ( !m_pD3DPERF_GetStatus ) {
        return ( 0 );
    }

    return ( m_pD3DPERF_GetStatus() );
}

extern "C" BOOL WINAPI D3DPERF_QueryRepeatFrame( void ) {
    const char l_message[] = "D3DPERF_QueryRepeatFrame ()\n";

    _useCallback( "log$transaction$query", l_message );
    _useCallback( "log$transaction$commit" );

    if ( !m_pD3DPERF_QueryRepeatFrame ) {
        return ( false );
    }

    return ( m_pD3DPERF_QueryRepeatFrame() );
}

extern "C" void WINAPI D3DPERF_SetMarker( D3DCOLOR col, LPCWSTR wszName ) {
    const char l_message[] = "D3DPERF_SetMarker ( col, wszName )\n";

    _useCallback( "log$transaction$query", l_message );
    _useCallback( "log$transaction$commit" );

    if ( m_pD3DPERF_SetMarker ) {
        return ( m_pD3DPERF_SetMarker( col, wszName ) );
    }
}

extern "C" void WINAPI D3DPERF_SetOptions( DWORD dwOptions ) {
    const char l_message[] = "D3DPERF_SetOptions ( dwOptions )\n";

    _useCallback( "log$transaction$query", l_message );
    _useCallback( "log$transaction$commit" );

    if ( m_pD3DPERF_SetOptions ) {
        return ( m_pD3DPERF_SetOptions( dwOptions ) );
    }
}

extern "C" void WINAPI D3DPERF_SetRegion( D3DCOLOR col, LPCWSTR wszName ) {
    const char l_message[] = "D3DPERF_SetRegion ( col, wszName )\n";

    _useCallback( "log$transaction$query", l_message );
    _useCallback( "log$transaction$commit" );

    if ( m_pD3DPERF_SetRegion ) {
        return ( m_pD3DPERF_SetRegion( col, wszName ) );
    }
}

extern "C" HRESULT WINAPI DebugSetLevel( DWORD dw1 ) {
    const char l_message[] = "DebugSetLevel ( dw1 )\n";

    _useCallback( "log$transaction$query", l_message );
    _useCallback( "log$transaction$commit" );

    if ( !m_pDebugSetLevel ) {
        return ( E_FAIL );
    }

    return ( m_pDebugSetLevel( dw1 ) );
}

extern "C" void WINAPI DebugSetMute( void ) {
#if 0
    const char l_message[] = "DebugSetMute ()\n";

    _useCallback( "log$transaction$query", l_message );
    _useCallback( "log$transaction$commit" );
#endif

    if ( m_pDebugSetMute ) {
        return ( m_pDebugSetMute() );
    }
}

extern "C" int WINAPI Direct3D9EnableMaximizedWindowedModeShim( BOOL mEnable ) {
    const char l_message[] =
        "Direct3D9EnableMaximizedWindowedModeShim ( mEnable )\n";

    _useCallback( "log$transaction$query", l_message );
    _useCallback( "log$transaction$commit" );

    if ( !m_pDirect3D9EnableMaximizedWindowedModeShim ) {
        return ( 0 );
    }

    return ( m_pDirect3D9EnableMaximizedWindowedModeShim( mEnable ) );
}

extern "C" IDirect3D9* WINAPI Direct3DCreate9( UINT SDKVersion ) {
    const char l_message[] = "Direct3DCreate9 ( SDKVersion )\n";

    _useCallback( "log$transaction$query", l_message );
    _useCallback( "log$transaction$commit" );

    if ( !m_pDirect3DCreate9 ) {
        return ( nullptr );
    }

    IDirect3D9* pD3D9 = m_pDirect3DCreate9( SDKVersion );

    if ( pD3D9 ) {
        return ( new m_IDirect3D9Ex( ( IDirect3D9Ex* )pD3D9, IID_IDirect3D9 ) );
    }

    return ( nullptr );
}

extern "C" HRESULT WINAPI Direct3DCreate9Ex( UINT SDKVersion,
                                             IDirect3D9Ex** ppD3D ) {
    const char l_message[] = "Direct3DCreate9Ex ( SDKVersion, ppD3D )\n";

    _useCallback( "log$transaction$query", l_message );
    _useCallback( "log$transaction$commit" );

    if ( !m_pDirect3DCreate9Ex ) {
        return ( E_FAIL );
    }

    HRESULT hr = m_pDirect3DCreate9Ex( SDKVersion, ppD3D );

    if ( SUCCEEDED( hr ) && ppD3D ) {
        *ppD3D = new m_IDirect3D9Ex( *ppD3D, IID_IDirect3D9Ex );
    }

    return ( hr );
}
