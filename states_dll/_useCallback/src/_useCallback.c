#include "_useCallback.h"

#include <omp.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#if defined( __cplusplus )

extern "C" {

#endif

uint16_t __useCallback( const char* _callbackName,
                        const size_t _callbackArgumentsCount,
                        ... ) {
#if defined( LOG_USE )

    printf( "Callback name : %s\n", _callbackName );
    printf( "Callback arguments length : %lu\n", _callbackArgumentsCount );

#endif

    if ( !_callbackArgumentsCount ) {
        return ( g_useCallback( _callbackName, NULL ) );
    }

    va_list l_arguments;
    va_start( l_arguments, _callbackArgumentsCount );

    void** l_callbackArguments =
        ( void** )malloc( _callbackArgumentsCount * sizeof( void* ) );

#pragma omp simd
    for ( size_t _index = 0; _index < _callbackArgumentsCount; _index++ ) {
        l_callbackArguments[ _index ] = va_arg( l_arguments, void* );
    }

    va_end( l_arguments );

    const uint16_t l_result =
        g_useCallback( _callbackName, l_callbackArguments );

    free( l_callbackArguments );

    return ( l_result );
}

#if defined( __cplusplus )
}

#endif
