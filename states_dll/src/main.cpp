#include <errno.h>
#include <omp.h>
#include <stdfunc.h>
#include <stdint.h>

#if ( defined( LOG_ADD ) || defined( LOG_USE ) )

#include <stdio.h>

#endif

#include <string>

#if defined( USE_BYTELL_HASH_MAP )

#include "bytell_hash_map.hpp"

#else

#include <map>

#endif

#if ( defined( LOG_ADD ) || defined( LOG_USE ) )

#define print( _text ) printf( _text "\n" )

#endif

typedef uint16_t addonCallbackFunction_t( void** );

#if defined( USE_BYTELL_HASH_MAP )

static ska::bytell_hash_map< std::string, addonCallbackFunction_t** >
    g_callbackFunctionAddresses;

#else

static std::map< std::string, addonCallbackFunction_t** >
    g_callbackFunctionAddresses;

#endif

extern "C" bool addCallbacks(
    const char* _callbackName,
    const size_t _functionCount,
    const addonCallbackFunction_t** _functionAddresses,
    const bool _overwrite ) {
    bool l_returnValue = true;

#if defined( LOG_ADD )

    printf( "Callback [ %s ]\n", _callbackName );
    printf( "Function count : %lu\n", _functionCount );
    printf( "Function addresses : %p\n", _functionAddresses );
    printf( "Overwrite : %s\n", ( _overwrite ) ? ( "true" ) : ( "false" ) );

#endif

    addonCallbackFunction_t** l_callbacks;
    l_callbacks =
        ( addonCallbackFunction_t** )createArray( sizeof( uintptr_t ) );

    preallocateArray( ( void*** )( &l_callbacks ), _functionCount );

#if defined( LOG_ADD )

    printf( "Preallocated array with length : %lu\n" );

    print( "Starting to store function addresses" );

#endif

    const size_t l_callbacksLength = arrayLength( l_callbacks );
    addonCallbackFunction_t** l_callbacksFirstElement =
        arrayFirstElementPointer( l_callbacks );
    addonCallbackFunction_t* const* l_callbacksEnd =
        ( l_callbacksFirstElement + l_callbacksLength );
    addonCallbackFunction_t** l_callback = l_callbacksFirstElement;

    addonCallbackFunction_t** l_functionAddressesFirstElement =
        _functionAddresses;
    addonCallbackFunction_t** l_functionAddress =
        l_functionAddressesFirstElement;

    while ( l_callback != l_callbacksEnd ) {
#if defined( LOG_ADD )

        printf( "Index at array : %lu\n",
                ( l_callback - l_callbacksFirstElement ) );

#endif

        ( *l_callback ) = *l_functionAddress;

#if defined( LOG_ADD )

        printf( "Function address : %lu\n", *l_callback );

#endif

        l_callback++;
        l_functionAddress++;
    }

#if defined( LOG_ADD )

    print( "Starting to emplace function addresses storage in global storage" );

#endif

    l_returnValue =
        g_callbackFunctionAddresses.emplace( _callbackName, l_callbacks )
            .second;

    if ( !l_returnValue ) {
#if defined( LOG_ADD )

        print( "Failed to emplace function addresses store in global storage" );

#endif

        if ( _overwrite ) {
#if defined( LOG_ADD )

            print(
                "Starting to overwrite function addresses in global storage" );

#endif

            free( g_callbackFunctionAddresses[ _callbackName ] );

            g_callbackFunctionAddresses[ _callbackName ] = l_callbacks;

            l_returnValue = true;

        } else {
            free( l_callbacks );
        }
    }

#if defined( LOG_ADD )

    print( "Function addresses store emplaced in global storage" );

#endif

    return ( l_returnValue );
}

extern "C" uint16_t useCallback( const char* _callbackName,
                                 void** _callbackArguments ) {
    uint16_t l_returnValue = 0;
    const auto l_callbacks = g_callbackFunctionAddresses.find( _callbackName );

    if ( l_callbacks == g_callbackFunctionAddresses.end() ) {
        l_returnValue = ENODATA;

    } else {
#if defined( LOG_USE )

        printf( "Callback name : %s\n", _callbackName );
        printf( "Callback arguments : %p\n", _callbackArguments );

#endif

        const size_t l_callbacksLength = arrayLength( l_callbacks->second );

#if defined( LOG_USE )

        printf( "Callback arguments length : %lu\n", l_callbacksLength );

#endif

        addonCallbackFunction_t** l_callbacksFirstElement =
            arrayFirstElementPointer( l_callbacks->second );
        addonCallbackFunction_t* const* l_callbacksEnd =
            ( l_callbacksFirstElement + l_callbacksLength );

#if defined( LOG_USE )

        printf( "Callback arguments first element : %p\n",
                l_callbacksFirstElement );
        printf( "Callback arguments end : %p\n", l_callbacksEnd );

#endif

        for ( addonCallbackFunction_t** _callback = l_callbacksFirstElement;
              _callback != l_callbacksEnd; _callback++ ) {
            const uint16_t l_result = ( *_callback )( _callbackArguments );

            if ( l_result ) {
#if defined( LOG_USE )

                printf( "Callback result : %u\n", l_result );

#endif

                l_returnValue = l_result;
            }
        }
    }

    return ( l_returnValue );
}
