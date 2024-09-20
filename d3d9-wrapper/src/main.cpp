#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <stdint.h>

#if defined( LOG_BOOT )

#include <stdio.h>

#endif

#include <string>

#include "_useCallback.h"
#include "d3d9.h"
#include "d3dx9.h"
#include "helpers.h"

#if defined( LOG_BOOT )

#define print( _text ) printf( _text "\n" )

#endif

static size_t l_renderCallsPerFrame = 0;
static const size_t l_result = 0;

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

HMODULE g_d3d9dll = NULL;
HMODULE g_justAnotherModloaderDll = NULL;

useCallbackFunction_t g_useCallback = NULL;

HRESULT m_IDirect3DDevice9Ex::Present( CONST RECT* pSourceRect,
                                       CONST RECT* pDestRect,
                                       HWND hDestWindowOverride,
                                       CONST RGNDATA* pDirtyRegion ) {
    l_renderCallsPerFrame = 0;

    const char l_message[] =
        "m_IDirect3DDevice9Ex Present ( pSourceRect, pDestRect, "
        "hDestWindowOverride, pDirtyRegion )\n";

#if 0
    _useCallback( "log$transaction$query", l_message );
#endif
#if 0
    _useCallback( "log$transaction$commit" );
#endif

#if 0
    uint16_t l_result =
        _useCallback( "IDirect3DDevice9Ex$Present", pSourceRect, pDestRect,
                      hDestWindowOverride, pDirtyRegion );
#endif

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

#if 0
    _useCallback( "log$transaction$query", l_message );
#endif
#if 0
    _useCallback( "log$transaction$commit" );
#endif

#if 0
    uint16_t l_result =
        _useCallback( "IDirect3DDevice9Ex$PresentEx", pSourceRect, pDestRect,
                      hDestWindowOverride, pDirtyRegion, &dwFlags );
#endif

    if ( ( l_result != 0 ) && ( l_result != ENODATA ) ) {
        return ( E_FAIL );
    }

    return ( ProxyInterface->PresentEx(
        pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion, dwFlags ) );
}

HRESULT m_IDirect3DDevice9Ex::EndScene( void ) {
    l_renderCallsPerFrame++;

    const char l_message[] = "m_IDirect3DDevice9Ex EndScene ()\n";

#if 0
    _useCallback( "log$transaction$query", l_message );
#endif
#if 0
    _useCallback( "log$transaction$commit" );
#endif

#if 0
    uint16_t l_result = _useCallback( "IDirect3DDevice9Ex$EndScene" );
#endif

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

#if 0
    _useCallback( "log$transaction$query", l_message );
#endif
#if 0
    _useCallback( "log$transaction$commit" );
#endif

#if 0
    uint16_t l_result = _useCallback(
        "IDirect3D9Ex$CreateDevice", &Adapter, &DeviceType, &hFocusWindow,
        &BehaviorFlags, &pPresentationParameters, &ppReturnedDeviceInterface );
#endif

    if ( ( l_result != 0 ) && ( l_result != ENODATA ) ) {
        return ( E_FAIL );
    }

    return ( hr );
}

HRESULT m_IDirect3DDevice9Ex::Reset(
    D3DPRESENT_PARAMETERS* pPresentationParameters ) {
    const char l_message[] =
        "m_IDirect3DDevice9Ex Reset ( pPresentationParameters )\n";

#if 0
    _useCallback( "log$transaction$query", l_message );
#endif
#if 0
    _useCallback( "log$transaction$commit" );
#endif

#if 0
    uint16_t l_result =
        _useCallback( "IDirect3DDevice9Ex$PreReset", pPresentationParameters );
#endif

    if ( ( l_result != 0 ) && ( l_result != ENODATA ) ) {
        return ( E_FAIL );
    }

    auto hRet = ProxyInterface->Reset( pPresentationParameters );

#if 0
    l_result =
        _useCallback( "IDirect3DDevice9Ex$PostReset", pPresentationParameters );
#endif

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

#if 0
    _useCallback( "log$transaction$query", l_message );
#endif
#if 0
    _useCallback( "log$transaction$commit" );
#endif

#if 0
    uint16_t l_result =
        _useCallback( "IDirect3D9Ex$CreateDeviceEx", &Adapter, &DeviceType,
                      hFocusWindow, &BehaviorFlags, pPresentationParameters,
                      pFullscreenDisplayMode, ppReturnedDeviceInterface );
#endif

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

#if 0
    _useCallback( "log$transaction$query", l_message );
#endif
#if 0
    _useCallback( "log$transaction$commit" );
#endif

#if 0
    uint16_t l_result =
        _useCallback( "IDirect3DDevice9Ex$PreResetEx", pPresentationParameters,
                      pFullscreenDisplayMode );
#endif

    if ( ( l_result != 0 ) && ( l_result != ENODATA ) ) {
        return ( E_FAIL );
    }

    auto hRet = ProxyInterface->ResetEx( pPresentationParameters,
                                         pFullscreenDisplayMode );

#if 0
    l_result = _useCallback( "IDirect3DDevice9Ex$PostResetEx",
                             pPresentationParameters, pFullscreenDisplayMode );
#endif

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

#if defined( LOG_ALLOCATE_CONSOLE )
            AllocConsole();
            freopen( "CONOUT$", "w", stdout );
#endif

#if defined( LOG_BOOT )
            print( "Starting to load d3d9.dll" );
#endif

            bool l_d3d9LoadResult = loadDll( "d3d9.dll", &g_d3d9dll );

            if ( l_d3d9LoadResult ) {
#if defined( LOG_BOOT )
                print( "Loaded d3d9.dll" );
#endif

                if ( g_d3d9dll ) {
#if defined( LOG_BOOT )
                    print( "Starting to locate d3d9.dll functions" );
#endif

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

#if defined( LOG_BOOT )
                print( "Starting to load just_another_modloader.dll" );
#endif

                g_justAnotherModloaderDll =
                    LoadLibraryA( "just_another_modloader.dll" );

                if ( !g_justAnotherModloaderDll ) {
#if defined( LOG_BOOT )
                    print( "Failed to load just_another_modloader.dll" );
#endif

                    exit( 1 );
                }

#if defined( LOG_BOOT )
                print( "Starting to locate states.dll" );
#endif

                HMODULE l_statesDll = GetModuleHandleA( "states.dll" );

                if ( !l_statesDll ) {
#if defined( LOG_BOOT )
                    print( "Failed to locate states.dll" );
#endif

                    exit( 1 );
                }

#if defined( LOG_BOOT )
                print( "Starting to locate \"useCallback\" in states.dll" );
#endif

                g_useCallback = ( useCallbackFunction_t )GetProcAddress(
                    l_statesDll, "useCallback" );

                if ( !g_useCallback ) {
#if defined( LOG_BOOT )
                    print( "Failed to locate \"useCallback\" in states.dll" );
#endif

                    exit( 1 );
                }

            } else {
#if defined( LOG_BOOT )
                print( "Failed to load d3d9.dll" );
#endif

                exit( 1 );
            }

            break;
        }
    }

    return ( true );
}

extern "C" HRESULT WINAPI Direct3DShaderValidatorCreate9( void ) {
    const char l_message[] = "Direct3DShaderValidatorCreate9 ()\n";

#if 0
    _useCallback( "log$transaction$query", l_message );
#endif
#if 0
    _useCallback( "log$transaction$commit" );
#endif

    if ( !m_pDirect3DShaderValidatorCreate9 ) {
        return ( E_FAIL );
    }

    return ( m_pDirect3DShaderValidatorCreate9() );
}

extern "C" HRESULT WINAPI PSGPError( void ) {
    const char l_message[] = "PSGPError ()\n";

#if 0
    _useCallback( "log$transaction$query", l_message );
#endif
#if 0
    _useCallback( "log$transaction$commit" );
#endif

    if ( !m_pPSGPError ) {
        return ( E_FAIL );
    }

    return ( m_pPSGPError() );
}

extern "C" HRESULT WINAPI PSGPSampleTexture( void ) {
    const char l_message[] = "PSGPSampleTexture ()\n";

#if 0
    _useCallback( "log$transaction$query", l_message );
#endif
#if 0
    _useCallback( "log$transaction$commit" );
#endif

    if ( !m_pPSGPSampleTexture ) {
        return ( E_FAIL );
    }

    return ( m_pPSGPSampleTexture() );
}

extern "C" int WINAPI D3DPERF_BeginEvent( D3DCOLOR col, LPCWSTR wszName ) {
    const char l_message[] = "D3DPERF_BeginEvent ( col, wszName )\n";

#if 0
    _useCallback( "log$transaction$query", l_message );
#endif
#if 0
    _useCallback( "log$transaction$commit" );
#endif

    if ( !m_pD3DPERF_BeginEvent ) {
        return ( 0 );
    }

    return ( m_pD3DPERF_BeginEvent( col, wszName ) );
}

extern "C" int WINAPI D3DPERF_EndEvent( void ) {
    const char l_message[] = "D3DPERF_EndEvent ()\n";

#if 0
    _useCallback( "log$transaction$query", l_message );
#endif
#if 0
    _useCallback( "log$transaction$commit" );
#endif

    if ( !m_pD3DPERF_EndEvent ) {
        return ( 0 );
    }

    return ( m_pD3DPERF_EndEvent() );
}

extern "C" DWORD WINAPI D3DPERF_GetStatus( void ) {
    const char l_message[] = "D3DPERF_GetStatus ()\n";

#if 0
    _useCallback( "log$transaction$query", l_message );
#endif
#if 0
    _useCallback( "log$transaction$commit" );
#endif

    if ( !m_pD3DPERF_GetStatus ) {
        return ( 0 );
    }

    return ( m_pD3DPERF_GetStatus() );
}

extern "C" BOOL WINAPI D3DPERF_QueryRepeatFrame( void ) {
    const char l_message[] = "D3DPERF_QueryRepeatFrame ()\n";

#if 0
    _useCallback( "log$transaction$query", l_message );
#endif
#if 0
    _useCallback( "log$transaction$commit" );
#endif

    if ( !m_pD3DPERF_QueryRepeatFrame ) {
        return ( false );
    }

    return ( m_pD3DPERF_QueryRepeatFrame() );
}

extern "C" void WINAPI D3DPERF_SetMarker( D3DCOLOR col, LPCWSTR wszName ) {
    const char l_message[] = "D3DPERF_SetMarker ( col, wszName )\n";

#if 0
    _useCallback( "log$transaction$query", l_message );
#endif
#if 0
    _useCallback( "log$transaction$commit" );
#endif

    if ( m_pD3DPERF_SetMarker ) {
        return ( m_pD3DPERF_SetMarker( col, wszName ) );
    }
}

extern "C" void WINAPI D3DPERF_SetOptions( DWORD dwOptions ) {
    const char l_message[] = "D3DPERF_SetOptions ( dwOptions )\n";

#if 0
    _useCallback( "log$transaction$query", l_message );
#endif
#if 0
    _useCallback( "log$transaction$commit" );
#endif

    if ( m_pD3DPERF_SetOptions ) {
        return ( m_pD3DPERF_SetOptions( dwOptions ) );
    }
}

extern "C" void WINAPI D3DPERF_SetRegion( D3DCOLOR col, LPCWSTR wszName ) {
    const char l_message[] = "D3DPERF_SetRegion ( col, wszName )\n";

#if 0
    _useCallback( "log$transaction$query", l_message );
#endif
#if 0
    _useCallback( "log$transaction$commit" );
#endif

    if ( m_pD3DPERF_SetRegion ) {
        return ( m_pD3DPERF_SetRegion( col, wszName ) );
    }
}

extern "C" HRESULT WINAPI DebugSetLevel( DWORD dw1 ) {
    const char l_message[] = "DebugSetLevel ( dw1 )\n";

#if 0
    _useCallback( "log$transaction$query", l_message );
#endif
#if 0
    _useCallback( "log$transaction$commit" );
#endif

    if ( !m_pDebugSetLevel ) {
        return ( E_FAIL );
    }

    return ( m_pDebugSetLevel( dw1 ) );
}

extern "C" void WINAPI DebugSetMute( void ) {
    const char l_message[] = "DebugSetMute ()\n";

#if 0
    _useCallback( "log$transaction$query", l_message );
#endif
#if 0
    _useCallback( "log$transaction$commit" );
#endif

    if ( m_pDebugSetMute ) {
        return ( m_pDebugSetMute() );
    }
}

extern "C" int WINAPI Direct3D9EnableMaximizedWindowedModeShim( BOOL mEnable ) {
    const char l_message[] =
        "Direct3D9EnableMaximizedWindowedModeShim ( mEnable )\n";

#if 0
    _useCallback( "log$transaction$query", l_message );
#endif
#if 0
    _useCallback( "log$transaction$commit" );
#endif

    if ( !m_pDirect3D9EnableMaximizedWindowedModeShim ) {
        return ( 0 );
    }

    return ( m_pDirect3D9EnableMaximizedWindowedModeShim( mEnable ) );
}

extern "C" IDirect3D9* WINAPI Direct3DCreate9( UINT SDKVersion ) {
    const char l_message[] = "Direct3DCreate9 ( SDKVersion )\n";

#if 0
    _useCallback( "log$transaction$query", l_message );
#endif
#if 0
    _useCallback( "log$transaction$commit" );
#endif

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

#if 0
    _useCallback( "log$transaction$query", l_message );
#endif
#if 0
    _useCallback( "log$transaction$commit" );
#endif

    if ( !m_pDirect3DCreate9Ex ) {
        return ( E_FAIL );
    }

    HRESULT hr = m_pDirect3DCreate9Ex( SDKVersion, ppD3D );

    if ( SUCCEEDED( hr ) && ppD3D ) {
        *ppD3D = new m_IDirect3D9Ex( *ppD3D, IID_IDirect3D9Ex );
    }

    return ( hr );
}
