#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <stdint.h>

#if defined( LOG_BOOT )

#include <stdio.h>

#endif // LOG_BOOT

#include <string>

#include "D3DPERF_BeginEvent.h"
#include "D3DPERF_EndEvent.h"
#include "D3DPERF_GetStatus.h"
#include "D3DPERF_QueryRepeatFrame.h"
#include "D3DPERF_SetMarker.h"
#include "D3DPERF_SetOptions.h"
#include "D3DPERF_SetRegion.h"
#include "DebugSetLevel.h"
#include "DebugSetMute.h"
#include "Direct3D9EnableMaximizedWindowedModeShim.h"
#include "Direct3DCreate9.h"
#include "Direct3DCreate9Ex.h"
#include "Direct3DShaderValidatorCreate9.h"
#include "PSGPError.h"
#include "PSGPSampleTexture.h"
#include "_useCallback.h"
#include "_useCallback.hpp"

#if defined( LOG_BOOT )

#define print( _text ) printf( _text "\n" )

#endif // LOG_BOOT

useCallbackFunction_t g_useCallback = NULL;
HMODULE g_d3d9dll = NULL;
HMODULE g_justAnotherModloaderDll = NULL;

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

extern "C" int __attribute__( ( stdcall ) ) DllMain( HMODULE hModule,
                                                     unsigned long dwReason,
                                                     void* lpReserved ) {
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
