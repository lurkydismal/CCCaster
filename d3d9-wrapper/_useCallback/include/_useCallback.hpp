#pragma once

#if defined( NO_CALLBACKS )

#undef __useCallback

#include <string>
#include <vector>

uint16_t __useCallback( const char* _callbackName,
                        const size_t _callbackArgumentsCount,
                        ... ) {
    std::string l_text;

    if ( _callbackName == "log$transaction$query" ) {
        va_list l_arguments;
        va_start( l_arguments, _callbackArgumentsCount );

        for ( size_t _index = 0; _index < _callbackArgumentsCount; _index++ ) {
            printf( "%s", ( va_arg( l_arguments, char* ) ) );
        }

        va_end( l_arguments );
    }

    printf( l_text.c_str() );

    return ( 0 );
}

#endif // NO_CALLBACKS
