#include <errno.h>
#include <omp.h>
#include <stdfunc.h>
#include <stdint.h>
#include <string.h>

#include <string>

#include "bytell_map.hpp"

typedef uint16_t addonCallbackFunction_t( void** );

ska::bytell_hash_map< std::string, addonCallbackFunction_t** >
    g_callbackFunctionAddresses;

extern "C" bool addCallbacks( const char* _callbackName,
                              const size_t _functionCount,
                              const uintptr_t* _functionAddresses,
                              const bool _overwrite ) {
    bool l_returnValue = true;
    addonCallbackFunction_t** l_callbacks;
    l_callbacks =
        ( addonCallbackFunction_t** )createArray( sizeof( uintptr_t ) );
    preallocateArray( ( void*** )( &l_callbacks ),
                      ( _functionCount + 1 ) * sizeof( uintptr_t ) );

    for ( size_t _functionIndex = 1; _functionIndex < ( _functionCount + 1 );
          _functionIndex++ ) {
        printf( "%s: %d = %p : %b\n", _callbackName, _functionCount,
                _functionAddresses[ _functionIndex - 1 ], _overwrite );
        insertIntoArrayByIndex(
            ( void*** )( &l_callbacks ), _functionIndex,
            ( void* )( reinterpret_cast< addonCallbackFunction_t* >(
                _functionAddresses[ _functionIndex - 1 ] ) ),
            sizeof( uintptr_t ) );
    }

    l_returnValue =
        g_callbackFunctionAddresses.emplace( _callbackName, l_callbacks )
            .second;

    if ( !l_returnValue ) {
        if ( _overwrite ) {
            g_callbackFunctionAddresses[ _callbackName ] = l_callbacks;

            l_returnValue = true;

        } else {
            free( l_callbacks );
        }
    }

    return ( l_returnValue );
}

extern "C" uint16_t useCallback( const char* _callbackName,
                                 void** _callbackArguments ) {
    uint16_t l_returnValue = 0;
    const addonCallbackFunction_t** l_callbacks =
        g_callbackFunctionAddresses.find( _callbackName )->second;

    if ( ( size_t )l_callbacks == std::numeric_limits< size_t >::max() ) {
        l_returnValue = ENODATA;

    } else {
        const size_t l_callbacksLength = arrayLength( l_callbacks );

        for ( size_t _callbackIndex = 1;
              _callbackIndex < ( l_callbacksLength + 1 ); _callbackIndex++ ) {
            printf( "333 %s %d = %d\n", _callbackName, l_callbacksLength,
                    _callbackIndex );

            const addonCallbackFunction_t* _callback =
                l_callbacks[ _callbackIndex ];
            printf( "222 %p : %p\n", _callback );
            const uint16_t l_result = _callback( _callbackArguments );

            if ( l_result ) {
                l_returnValue = l_result;

                break;
            }
        }
    }

    return ( l_returnValue );
}
