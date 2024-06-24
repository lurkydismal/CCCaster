#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <cstdint>
#include <fstream>
#include <map>
#include <string>
#include <vector>

#include "icecream.h"

#define LOG_FILE_NAME "log_trace"
#define LOG_FILE_EXTENSION "txt"

enum logLevel_t { TRACE = 0, DEBUG, INFO, WARN, ERR, FATAL };

typedef uint16_t addonCallbackFunction_t( void** );

std::map< std::string, std::vector< addonCallbackFunction_t* > >
    g_callbackFunctionAddresses;

static bool _addCallbacks( std::string const& _callbackName,
                           size_t _functionCount,
                           uintptr_t* _functionAddresses,
                           const bool _overwrite = false ) {
    bool l_returnValue = true;
    std::vector< addonCallbackFunction_t* > l_callbacks;

#if defined( TRACE )

    ic();
    ic_str( _callbackName.c_str() );
    ic_int( _functionCount, _overwrite );
    ic_ptr( _functionAddresses );

#endif

    for ( uint32_t _functionIndex = 0; _functionIndex < _functionCount;
          _functionIndex++ ) {
        l_callbacks.emplace_back(
            ( reinterpret_cast< addonCallbackFunction_t* >(
                _functionAddresses[ _functionIndex ] ) ) );
    }

    auto l_result =
        g_callbackFunctionAddresses.emplace( _callbackName, l_callbacks );

    l_returnValue = l_result.second;

    if ( ( _overwrite ) && ( !l_returnValue ) ) {
        g_callbackFunctionAddresses[ _callbackName ] = l_callbacks;

        l_returnValue = true;
    }

    return ( l_returnValue );
}

extern "C" bool __declspec( dllexport )
    addCallbacks( const char* _callbackName,
                  size_t _functionCount,
                  uintptr_t* _functionAddresses,
                  const bool _overwrite = false ) {
    bool l_returnValue = true;
    std::string l_callbackName( _callbackName );

    l_returnValue =
        _addCallbacks( l_callbackName, _functionCount, _functionAddresses );

    return ( l_returnValue );
}

static uint16_t _useCallback( std::string const& _callbackName,
                              void** _callbackArguments = NULL ) {
    uint16_t l_returnValue = 0;
    const auto l_callbacks = g_callbackFunctionAddresses.find( _callbackName );

#if defined( TRACE )

    ic();
    ic_str( _callbackName.c_str() );
    ic_ptr( _callbackArguments );

#endif

    if ( l_callbacks == g_callbackFunctionAddresses.end() ) {
        l_returnValue = ENODATA;

    } else {
        for ( const auto _callback : l_callbacks->second ) {
            const uint16_t l_result = _callback( _callbackArguments );

            if ( l_result ) {
                l_returnValue = l_result;
            }
        }
    }

    return ( l_returnValue );
}

extern "C" uint16_t __declspec( dllexport )
    useCallback( const char* _callbackName, void** _callbackArguments = NULL ) {
    uint16_t l_returnValue = 0;
    std::string l_callbackName( _callbackName );

    l_returnValue = _useCallback( l_callbackName, _callbackArguments );

    return ( l_returnValue );
}

extern "C" BOOL WINAPI DllMain( HMODULE hModule,
                                DWORD dwReason,
                                LPVOID lpReserved ) {
    bool l_returnValue = true;

    switch ( dwReason ) {
        case DLL_PROCESS_ATTACH: {
#if defined( TRACE )

            FILE* l_fileHandle =
                fopen( ( std::string( LOG_FILE_NAME ) + std::string( "." ) +
                         std::string( LOG_FILE_EXTENSION ) )
                           .c_str(),
                       "wb" );

            if ( l_fileHandle ) {
                log_add_fp( l_fileHandle, DEBUG );
            }

#endif

            break;
        }
    }

    return ( l_returnValue );
}
