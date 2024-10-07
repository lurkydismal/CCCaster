#include <errno.h>
#include <omp.h>
#include <stdfunc.h>
#include <stdint.h>

#if ( defined( LOG_ADD ) || defined( LOG_USE ) )

#include <stdio.h>

#endif

#include <map>
#include <string>

#if ( defined( LOG_ADD ) || defined( LOG_USE ) )

#define print( _text ) printf( _text "\n" )

#endif

typedef uint16_t addonCallbackFunction_t( void** );

static std::map< std::string, addonCallbackFunction_t** >
    g_callbackFunctionAddresses;

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

    FOR_ARRAY( addonCallbackFunction_t**, l_callbacks ) {
        const size_t l_index =
            ( _element - arrayFirstElementPointer( l_callbacks ) );
#if defined( LOG_ADD )

        printf( "Index at array : %lu\n", l_index );

#endif

        *_element = _functionAddresses[ l_index ];

#if defined( LOG_ADD )

        printf( "Function address : %lu\n", *_element );

#endif
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
        printf( "Callback arguments length : %lu\n",
                arrayLength( l_callbacks->second ) );

#endif

        FOR_ARRAY( addonCallbackFunction_t**, l_callbacks->second ) {
#if defined( LOG_USE )

            printf( "Callback : %p\n", *_element );

#endif

            const uint16_t l_result = ( *_element )( _callbackArguments );

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
