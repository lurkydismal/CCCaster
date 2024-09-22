#pragma once

#include <wtypes.h>

#include <stdint.h>
#include <stdlib.h>
#include <winbase.h>

#define _useCallbackInitialize()                                 \
    do {                                                         \
        void* l_statesDll = GetModuleHandleA( "states.dll" );    \
                                                                 \
        if ( !l_statesDll ) {                                    \
            return ( 1 );                                        \
        }                                                        \
                                                                 \
        g_useCallback = ( useCallbackFunction_t )GetProcAddress( \
            l_statesDll, "useCallback" );                        \
                                                                 \
        if ( !g_useCallback ) {                                  \
            return ( 1 );                                        \
        }                                                        \
    } while ( 0 );

#define ARGUMENTS_COUNT( ... )                               \
    ( ( sizeof( ( const void*[] ){ NULL, ##__VA_ARGS__ } ) / \
        sizeof( const void* ) ) -                            \
      1 )

#define _useCallback( _callbackName, ... )                         \
    __useCallback( _callbackName,                                  \
                   ( const size_t )ARGUMENTS_COUNT( __VA_ARGS__ ), \
                   ##__VA_ARGS__ )

#if defined( __cplusplus )

extern "C" {

#endif

typedef uint16_t( __cdecl* useCallbackFunction_t )( const char* _callbackName,
                                                    void** _callbackArguments );
extern useCallbackFunction_t g_useCallback;

uint16_t __useCallback( const char* _callbackName,
                        const size_t _callbackArgumentsCount,
                        ... );

#if defined( __cplusplus )
}

#endif
