#include <cstdarg>

#include "_useCallback.hpp"

uint16_t _useCallback( std::string const& _callbackName,
                       const size_t _callbackArgumentsCount,
                       ... ) {
    if ( !_callbackArgumentsCount ) {
        return ( g_useCallback( _callbackName.c_str(), NULL ) );
    }

    va_list l_arguments;
    va_start( l_arguments, _callbackArgumentsCount );

    void** l_callbackArguments = new void*[ _callbackArgumentsCount ];

    for ( size_t _index = 0; _index < _callbackArgumentsCount; _index++ ) {
        l_callbackArguments[ _index ] = va_arg( l_arguments, void* );
    }

    va_end( l_arguments );

    uint16_t l_result =
        g_useCallback( _callbackName.c_str(), l_callbackArguments );

    delete l_callbackArguments;

    return ( l_result );
}
