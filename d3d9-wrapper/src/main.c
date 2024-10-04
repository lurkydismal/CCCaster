#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <stdint.h>
#include <stdbool.h>

#if ( defined( LOG_BOOT ) || defined( NO_CALLBACKS ) || defined( LOG_ALLOCATE_CONSOLE ) )

#include <stdio.h>

#endif // LOG_BOOT

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

#define ATTACH 1

useCallbackFunction_t g_useCallback;

static inline HMODULE loadSystemDll( const char* _DllName ) {
    HMODULE l_returnValue = NULL;
    char* l_systemDirectoryPath = (char*)malloc( MAX_PATH );

    GetSystemDirectoryA( l_systemDirectoryPath, MAX_PATH );

    const size_t l_systemDirectoryPathLength = strlen( l_systemDirectoryPath );
    const size_t l_DllNameLength = strlen( _DllName );

    l_systemDirectoryPath = (char*)realloc( l_systemDirectoryPath, ( l_systemDirectoryPathLength + 1 + l_DllNameLength + 1 ) );

    if ( ( l_systemDirectoryPathLength + 1 ) < MAX_PATH ) {
        {
            char* l_stringWriteIndex = ( l_systemDirectoryPath + l_systemDirectoryPathLength );

            memcpy( l_stringWriteIndex, "/", 1 );

            l_stringWriteIndex += 1;

            memcpy( l_stringWriteIndex, _DllName, l_DllNameLength );

            l_stringWriteIndex += l_DllNameLength;

            *l_stringWriteIndex = '\0';

        }

        l_returnValue = LoadLibraryA( l_systemDirectoryPath );
    }

    free( l_systemDirectoryPath );

    return ( l_returnValue );
}

static inline void hookD3D9( HMODULE _d3d9dll ) {
#if defined( LOG_BOOT )

    print( "Starting to locate d3d9.dll functions" );

#endif // LOG_BOOT

#define GET_ADDRESS( _functionName ) \
    m_p##_functionName =             \
    ( _functionName##Proc )GetProcAddress( _d3d9dll, #_functionName );

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

#if defined( LOG_BOOT )

        print( "Functions from d3d9.dll successfully hooked" );

#endif // LOG_BOOT
}

int32_t __attribute__( ( stdcall ) ) DllMain( void* _handle,
                                              unsigned long _reason,
                                              void* _ ) {
    bool l_returnValue = true;

    if ( _reason == ATTACH ) {
#if defined( LOG_ALLOCATE_CONSOLE )

        AllocConsole();
        freopen( "CONOUT$", "w", stdout );

#endif // LOG_ALLOCATE_CONSOLE

#if defined( LOG_BOOT )

        print( "Starting to load d3d9.dll" );

#endif // LOG_BOOT

        HMODULE l_d3d9dll = loadSystemDll( "d3d9.dll" );

        if ( l_d3d9dll ) {
#if defined( LOG_BOOT )

            print( "Loaded d3d9.dll" );

#endif // LOG_BOOT

            hookD3D9( l_d3d9dll );

            // ES_DISPLAY_REQUIRED - Forces the display to be on by resetting the display idle timer.
            // ES_SYSTEM_REQUIRED - Forces the system to be in the working state by resetting the system idle timer.
            uint32_t l_executionThreadStateFlags = ( ES_DISPLAY_REQUIRED | ES_SYSTEM_REQUIRED );

            SetThreadExecutionState( l_executionThreadStateFlags );

#if !defined( NO_CALLBACKS )

#if defined( LOG_BOOT )

            print( "Starting to load just_another_modloader.dll" );

#endif // LOG_BOOT

            HMODULE l_justAnotherModloaderDll =
                LoadLibraryA( "just_another_modloader.dll" );

            if ( !l_justAnotherModloaderDll ) {
#if defined( LOG_BOOT )

                print( "Failed to load just_another_modloader.dll" );

#endif // LOG_BOOT

                l_returnValue = false;
            }

#if defined( LOG_BOOT )

            print( "Starting to locate states.dll" );

#endif // LOG_BOOT

            HMODULE l_statesDll = GetModuleHandleA( "states.dll" );

            if ( !l_statesDll ) {
#if defined( LOG_BOOT )

                print( "Failed to locate states.dll" );

#endif // LOG_BOOT

                l_returnValue = false;
            }

#if defined( LOG_BOOT )

            print( "Starting to locate \"useCallback\" in states.dll" );

#endif // LOG_BOOT

            g_useCallback = ( useCallbackFunction_t )GetProcAddress(
                    l_statesDll, "useCallback" );

            if ( !g_useCallback ) {
#if defined( LOG_BOOT )

                print( "Failed to locate \"useCallback\" in states.dll" );

#endif // LOG_BOOT

                l_returnValue = false;
            }

#if defined( LOG_BOOT )

            print( "Located \"useCallback\" in states.dll" );

#endif // LOG_BOOT

#endif // ! NO_CALLBACKS

        } else {
#if defined( LOG_BOOT )

            print( "Failed to load d3d9.dll" );

#endif // LOG_BOOT

            l_returnValue = false;
        }

#if defined( LOG_ALLOCATE_CONSOLE )

        getchar();

#endif

    }

    return ( l_returnValue );
}
