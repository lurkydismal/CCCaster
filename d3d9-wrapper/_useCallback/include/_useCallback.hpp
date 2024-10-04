#pragma once

#if defined( NO_CALLBACKS )

#if defined( _useCallback )

#undef _useCallback

#endif // _useCallback

#define _useCallback( _text, ... ) {}

#if 0

#include <stdarg.h>
#include <stdio.h>

uint16_t __useCallback( const char* _callbackName,
                        const size_t _callbackArgumentsCount,
                        ... ) {
    if ( _callbackName == "log$transaction$query" ) {
        va_list l_arguments;
        va_start( l_arguments, _callbackArgumentsCount );

        for ( size_t _index = 0; _index < _callbackArgumentsCount; _index++ ) {
            printf( "%s", ( va_arg( l_arguments, char* ) ) );
        }

        va_end( l_arguments );
    }

    return ( 0 );
}

#endif

#endif // NO_CALLBACKS
