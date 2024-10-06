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

    printf( "_Callback name : %s\n", _callbackName );
    printf( "_Callback arguments length : %u\n", _callbackArgumentsCount );

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

#if defined( LOG_USE )

        printf( "_Callback argument %u pointer : %p\n", _index,
                l_callbackArguments[ _index ] );

#endif
    }

    va_end( l_arguments );

#if defined( LOG_USE )

    if ( !g_useCallback ) {
        printf( "g_useCallback pointer : %p\n", g_useCallback );
    }

#endif

    const uint16_t l_result =
        g_useCallback( _callbackName, l_callbackArguments );

#if defined( LOG_USE )

    if ( l_result ) {
        printf( "_Callback result : %u\n", l_result );
    }

#endif

    free( l_callbackArguments );

    return ( l_result );
}

#if defined( __cplusplus )
}

#endif
