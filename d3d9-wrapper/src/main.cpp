#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <stdint.h>

#if defined( LOG_BOOT )

#include <stdio.h>

#endif // LOG_BOOT

#include <string>

#include "_useCallback.h"
#include "_useCallback.hpp"
#include "d3d9.h"
#include "d3dx9.h"
#include "helpers.h"

#if defined( LOG_BOOT )

#define print( _text ) printf( _text "\n" )

#endif // LOG_BOOT

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

useCallbackFunction_t g_useCallback = NULL;
HMODULE g_d3d9dll = NULL;
HMODULE g_justAnotherModloaderDll = NULL;

extern "C" long __attribute__( ( stdcall ) ) Direct3DShaderValidatorCreate9(
    void ) {
#if defined( LOG_EXPORTED_CALLS )

    const char l_message[] = "Direct3DShaderValidatorCreate9 ()\n";

    _useCallback( "log$transaction$query", l_message );
    _useCallback( "log$transaction$commit" );

#endif // LOG_EXPORTED_CALLS

    if ( !m_pDirect3DShaderValidatorCreate9 ) {
        return ( E_FAIL );
    }

    return ( m_pDirect3DShaderValidatorCreate9() );
}

extern "C" long __attribute__( ( stdcall ) ) PSGPError( void ) {
#if defined( LOG_EXPORTED_CALLS )

    const char l_message[] = "PSGPError ()\n";

    _useCallback( "log$transaction$query", l_message );
    _useCallback( "log$transaction$commit" );

#endif // LOG_EXPORTED_CALLS

    if ( !m_pPSGPError ) {
        return ( E_FAIL );
    }

    return ( m_pPSGPError() );
}

extern "C" long __attribute__( ( stdcall ) ) PSGPSampleTexture( void ) {
#if defined( LOG_EXPORTED_CALLS )

    const char l_message[] = "PSGPSampleTexture ()\n";

    _useCallback( "log$transaction$query", l_message );
    _useCallback( "log$transaction$commit" );

#endif // LOG_EXPORTED_CALLS

    if ( !m_pPSGPSampleTexture ) {
        return ( E_FAIL );
    }

    return ( m_pPSGPSampleTexture() );
}

extern "C" int __attribute__( ( stdcall ) ) D3DPERF_BeginEvent(
    D3DCOLOR _color,
    LPCWSTR _eventName ) {
#if defined( LOG_EXPORTED_CALLS )

    const char l_message[] = "D3DPERF_BeginEvent ( %d, %s )\n";

    _useCallback( "log$transaction$query", l_message );
    _useCallback( "log$transaction$commit" );

#endif // LOG_EXPORTED_CALLS

    if ( !m_pD3DPERF_BeginEvent ) {
        return ( 0 );
    }

    return ( m_pD3DPERF_BeginEvent( _color, _eventName ) );
}

extern "C" int __attribute__( ( stdcall ) ) D3DPERF_EndEvent( void ) {
#if defined( LOG_EXPORTED_CALLS )

    const char l_message[] = "D3DPERF_EndEvent ()\n";

    _useCallback( "log$transaction$query", l_message );
    _useCallback( "log$transaction$commit" );

#endif // LOG_EXPORTED_CALLS

    if ( !m_pD3DPERF_EndEvent ) {
        return ( 0 );
    }

    return ( m_pD3DPERF_EndEvent() );
}

extern "C" unsigned long __attribute__( ( stdcall ) ) D3DPERF_GetStatus(
    void ) {
#if defined( LOG_EXPORTED_CALLS )

    const char l_message[] = "D3DPERF_GetStatus ()\n";

    _useCallback( "log$transaction$query", l_message );
    _useCallback( "log$transaction$commit" );

#endif // LOG_EXPORTED_CALLS

    if ( !m_pD3DPERF_GetStatus ) {
        return ( 0 );
    }

    return ( m_pD3DPERF_GetStatus() );
}

extern "C" BOOL __attribute__( ( stdcall ) ) D3DPERF_QueryRepeatFrame( void ) {
#if defined( LOG_EXPORTED_CALLS )

    const char l_message[] = "D3DPERF_QueryRepeatFrame ()\n";

    _useCallback( "log$transaction$query", l_message );
    _useCallback( "log$transaction$commit" );

#endif // LOG_EXPORTED_CALLS

    if ( !m_pD3DPERF_QueryRepeatFrame ) {
        return ( false );
    }

    return ( m_pD3DPERF_QueryRepeatFrame() );
}

extern "C" void __attribute__( ( stdcall ) ) D3DPERF_SetMarker(
    D3DCOLOR _color,
    LPCWSTR _eventName ) {
#if defined( LOG_EXPORTED_CALLS )

    const char l_message[] = "D3DPERF_SetMarker ( %d, %s )\n";

    _useCallback( "log$transaction$query", l_message );
    _useCallback( "log$transaction$commit" );

#endif // LOG_EXPORTED_CALLS

    if ( m_pD3DPERF_SetMarker ) {
        return ( m_pD3DPERF_SetMarker( _color, _eventName ) );
    }
}

extern "C" void __attribute__( ( stdcall ) ) D3DPERF_SetOptions(
    unsigned long _options ) {
#if defined( LOG_EXPORTED_CALLS )

    const char l_message[] = "D3DPERF_SetOptions ( %d )\n";

    _useCallback( "log$transaction$query", l_message );
    _useCallback( "log$transaction$commit" );

#endif // LOG_EXPORTED_CALLS

    if ( m_pD3DPERF_SetOptions ) {
        return ( m_pD3DPERF_SetOptions( _options ) );
    }
}

extern "C" void __attribute__( ( stdcall ) ) D3DPERF_SetRegion(
    D3DCOLOR _color,
    LPCWSTR _eventName ) {
#if defined( LOG_EXPORTED_CALLS )

    const char l_message[] = "D3DPERF_SetRegion ( %d, %s )\n";

    _useCallback( "log$transaction$query", l_message );
    _useCallback( "log$transaction$commit" );

#endif // LOG_EXPORTED_CALLS

    if ( m_pD3DPERF_SetRegion ) {
        return ( m_pD3DPERF_SetRegion( _color, _eventName ) );
    }
}

extern "C" long __attribute__( ( stdcall ) ) DebugSetLevel(
    unsigned long _level ) {
#if defined( LOG_EXPORTED_CALLS )

    const char l_message[] = "DebugSetLevel ( %d )\n";

    _useCallback( "log$transaction$query", l_message );
    _useCallback( "log$transaction$commit" );

#endif // LOG_EXPORTED_CALLS

    if ( !m_pDebugSetLevel ) {
        return ( E_FAIL );
    }

    return ( m_pDebugSetLevel( _level ) );
}

extern "C" void __attribute__( ( stdcall ) ) DebugSetMute( void ) {
#if defined( LOG_EXPORTED_CALLS )

    const char l_message[] = "DebugSetMute ()\n";

    _useCallback( "log$transaction$query", l_message );
    _useCallback( "log$transaction$commit" );

#endif // LOG_EXPORTED_CALLS

    if ( m_pDebugSetMute ) {
        return ( m_pDebugSetMute() );
    }
}

extern "C" int __attribute__( ( stdcall ) )
Direct3D9EnableMaximizedWindowedModeShim( BOOL mEnable ) {
#if defined( LOG_EXPORTED_CALLS )

    const char l_message[] =
        "Direct3D9EnableMaximizedWindowedModeShim ( %d )\n";

    _useCallback( "log$transaction$query", l_message );
    _useCallback( "log$transaction$commit" );

#endif // LOG_EXPORTED_CALLS

    if ( !m_pDirect3D9EnableMaximizedWindowedModeShim ) {
        return ( 0 );
    }

    return ( m_pDirect3D9EnableMaximizedWindowedModeShim( mEnable ) );
}

extern "C" IDirect3D9* __attribute__( ( stdcall ) ) Direct3DCreate9(
    unsigned int SDKVersion ) {
#if defined( LOG_EXPORTED_CALLS )

    const char l_message[] = "Direct3DCreate9 ( %lu )\n";

    _useCallback( "log$transaction$query", l_message );
    _useCallback( "log$transaction$commit" );

#endif // LOG_EXPORTED_CALLS

    if ( !m_pDirect3DCreate9 ) {
        return ( nullptr );
    }

    IDirect3D9* pD3D9 = m_pDirect3DCreate9( SDKVersion );

    if ( pD3D9 ) {
        return ( new m_IDirect3D9Ex( ( IDirect3D9Ex* )pD3D9, IID_IDirect3D9 ) );
    }

    return ( nullptr );
}

extern "C" long __attribute__( ( stdcall ) ) Direct3DCreate9Ex(
    unsigned int SDKVersion,
    IDirect3D9Ex** ppD3D ) {
#if defined( LOG_EXPORTED_CALLS )

    const char l_message[] = "Direct3DCreate9Ex ( %lu, %p )\n";

    _useCallback( "log$transaction$query", l_message );
    _useCallback( "log$transaction$commit" );

#endif // LOG_EXPORTED_CALLS

    if ( !m_pDirect3DCreate9Ex ) {
        return ( E_FAIL );
    }

    long hr = m_pDirect3DCreate9Ex( SDKVersion, ppD3D );

    if ( SUCCEEDED( hr ) && ppD3D ) {
        *ppD3D = new m_IDirect3D9Ex( *ppD3D, IID_IDirect3D9Ex );
    }

    return ( hr );
}

static inline bool loadDll( const char* _DLLName, HMODULE* _moduleHandle ) {
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

extern "C" BOOL __attribute__( ( stdcall ) ) DllMain( HMODULE hModule,
                                                      unsigned long dwReason,
                                                      LPVOID lpReserved ) {
    switch ( dwReason ) {
        case DLL_PROCESS_ATTACH: {
#if defined( LOG_ALLOCATE_CONSOLE )

            AllocConsole();
            freopen( "CONOUT$", "w", stdout );

#endif // LOG_ALLOCATE_CONSOLE

#if defined( LOG_BOOT )

            print( "Starting to load d3d9.dll" );

#endif // LOG_BOOT

            bool l_d3d9LoadResult = loadDll( "d3d9.dll", &g_d3d9dll );

            if ( l_d3d9LoadResult ) {
#if defined( LOG_BOOT )

                print( "Loaded d3d9.dll" );

#endif // LOG_BOOT

                if ( g_d3d9dll ) {
#if defined( LOG_BOOT )

                    print( "Starting to locate d3d9.dll functions" );

#endif // LOG_BOOT

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

#if defined( LOG_BOOT )

                    print( "Functions from d3d9.dll successfully hooked" );

#endif // LOG_BOOT
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

#endif // LOG_BOOT

                g_justAnotherModloaderDll =
                    LoadLibraryA( "just_another_modloader.dll" );

                if ( !g_justAnotherModloaderDll ) {
#if defined( LOG_BOOT )

                    print( "Failed to load just_another_modloader.dll" );

#endif // LOG_BOOT

                    exit( 1 );
                }

#if defined( LOG_BOOT )

                print( "Starting to locate states.dll" );

#endif // LOG_BOOT

                HMODULE l_statesDll = GetModuleHandleA( "states.dll" );

                if ( !l_statesDll ) {
#if defined( LOG_BOOT )

                    print( "Failed to locate states.dll" );

#endif // LOG_BOOT

                    exit( 1 );
                }

#if !defined( NO_CALLBACKS )

#if defined( LOG_BOOT )

                print( "Starting to locate \"useCallback\" in states.dll" );

#endif // LOG_BOOT

                g_useCallback = ( useCallbackFunction_t )GetProcAddress(
                    l_statesDll, "useCallback" );

                if ( !g_useCallback ) {
#if defined( LOG_BOOT )

                    print( "Failed to locate \"useCallback\" in states.dll" );

#endif // LOG_BOOT

                    exit( 1 );
                }
#endif // ! NO_CALLBACKS

            } else {
#if defined( LOG_BOOT )

                print( "Failed to load d3d9.dll" );

#endif // LOG_BOOT

                exit( 1 );
            }

            break;
        }
    }

    return ( true );
}

long m_IDirect3DDevice9Ex::Present( CONST RECT* pSourceRect,
                                    CONST RECT* pDestRect,
                                    HWND hDestWindowOverride,
                                    CONST RGNDATA* pDirtyRegion ) {
#if defined( LOG_EXPORTED_CALLS )

    const char l_message[] =
        "m_IDirect3DDevice9Ex Present ( pSourceRect, pDestRect, "
        "hDestWindowOverride, pDirtyRegion )\n";

    _useCallback( "log$transaction$query", l_message );
    _useCallback( "log$transaction$commit" );

    uint16_t l_result =
        _useCallback( "IDirect3DDevice9Ex$Present", pSourceRect, pDestRect,
                      hDestWindowOverride, pDirtyRegion );

    if ( ( l_result != 0 ) && ( l_result != ENODATA ) ) {
        return ( E_FAIL );
    }

#endif // LOG_EXPORTED_CALLS

    l_renderCallsPerFrame = 0;

    return ( ProxyInterface->Present( pSourceRect, pDestRect,
                                      hDestWindowOverride, pDirtyRegion ) );
}

long m_IDirect3DDevice9Ex::PresentEx( THIS_ CONST RECT* pSourceRect,
                                      CONST RECT* pDestRect,
                                      HWND hDestWindowOverride,
                                      CONST RGNDATA* pDirtyRegion,
                                      unsigned long dwFlags ) {
#if defined( LOG_EXPORTED_CALLS )

    const char l_message[] =
        "m_IDirect3DDevice9Ex PresentEx ( pSourceRect, pDestRect, "
        "hDestWindowOverride, pDirtyRegion, dwFlags )\n";

    _useCallback( "log$transaction$query", l_message );
    _useCallback( "log$transaction$commit" );

    uint16_t l_result =
        _useCallback( "IDirect3DDevice9Ex$PresentEx", pSourceRect, pDestRect,
                      hDestWindowOverride, pDirtyRegion, &dwFlags );
#endif // LOG_EXPORTED_CALLS

    if ( ( l_result != 0 ) && ( l_result != ENODATA ) ) {
        return ( E_FAIL );
    }

    return ( ProxyInterface->PresentEx(
        pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion, dwFlags ) );
}

long m_IDirect3DDevice9Ex::EndScene( void ) {
    l_renderCallsPerFrame++;

    const char l_message[] = "m_IDirect3DDevice9Ex EndScene ()\n";

#if defined( LOG_EXPORTED_CALLS )

    _useCallback( "log$transaction$query", l_message );
    _useCallback( "log$transaction$commit" );

    uint16_t l_result = _useCallback( "IDirect3DDevice9Ex$EndScene" );

#endif // LOG_EXPORTED_CALLS

    if ( ( l_result != 0 ) && ( l_result != ENODATA ) ) {
        return ( E_FAIL );
    }

    return ( ProxyInterface->EndScene() );
}

long m_IDirect3D9Ex::CreateDevice(
    unsigned int Adapter,
    D3DDEVTYPE DeviceType,
    HWND hFocusWindow,
    unsigned long BehaviorFlags,
    D3DPRESENT_PARAMETERS* pPresentationParameters,
    IDirect3DDevice9** ppReturnedDeviceInterface ) {
    long hr = ProxyInterface->CreateDevice(
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

#if defined( LOG_EXPORTED_CALLS )

    _useCallback( "log$transaction$query", l_message );
    _useCallback( "log$transaction$commit" );

#endif // LOG_EXPORTED_CALLS

    uint16_t l_result = _useCallback(
        "IDirect3D9Ex$CreateDevice", &Adapter, &DeviceType, &hFocusWindow,
        &BehaviorFlags, &pPresentationParameters, &ppReturnedDeviceInterface );

    if ( ( l_result != 0 ) && ( l_result != ENODATA ) ) {
        return ( E_FAIL );
    }

    return ( hr );
}

long m_IDirect3DDevice9Ex::Reset(
    D3DPRESENT_PARAMETERS* pPresentationParameters ) {
    const char l_message[] =
        "m_IDirect3DDevice9Ex Reset ( pPresentationParameters )\n";

#if defined( LOG_EXPORTED_CALLS )

    _useCallback( "log$transaction$query", l_message );
    _useCallback( "log$transaction$commit" );

#endif // LOG_EXPORTED_CALLS

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

long m_IDirect3D9Ex::CreateDeviceEx(
    THIS_ unsigned int Adapter,
    D3DDEVTYPE DeviceType,
    HWND hFocusWindow,
    unsigned long BehaviorFlags,
    D3DPRESENT_PARAMETERS* pPresentationParameters,
    D3DDISPLAYMODEEX* pFullscreenDisplayMode,
    IDirect3DDevice9Ex** ppReturnedDeviceInterface ) {
    long hr = ProxyInterface->CreateDeviceEx(
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

#if defined( LOG_EXPORTED_CALLS )

    _useCallback( "log$transaction$query", l_message );
    _useCallback( "log$transaction$commit" );

#endif // LOG_EXPORTED_CALLS

    uint16_t l_result =
        _useCallback( "IDirect3D9Ex$CreateDeviceEx", &Adapter, &DeviceType,
                      hFocusWindow, &BehaviorFlags, pPresentationParameters,
                      pFullscreenDisplayMode, ppReturnedDeviceInterface );

    if ( ( l_result != 0 ) && ( l_result != ENODATA ) ) {
        return ( E_FAIL );
    }

    return ( hr );
}

long m_IDirect3DDevice9Ex::ResetEx(
    THIS_ D3DPRESENT_PARAMETERS* pPresentationParameters,
    D3DDISPLAYMODEEX* pFullscreenDisplayMode ) {
    const char l_message[] =
        "m_IDirect3DDevice9Ex ResetEx ( pPresentationParameters, "
        "pFullscreenDisplayMode )\n";

#if defined( LOG_EXPORTED_CALLS )

    _useCallback( "log$transaction$query", l_message );
    _useCallback( "log$transaction$commit" );

#endif // LOG_EXPORTED_CALLS

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
