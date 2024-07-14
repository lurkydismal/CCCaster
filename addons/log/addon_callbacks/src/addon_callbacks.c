#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <stdint.h>

#include "log.h"

int16_t __declspec( dllexport ) log$transaction$query(
    void** _callbackArguments ) {
    uint16_t l_returnCode = 0;
    const char* _string = ( const char* )_callbackArguments[ 0 ];
    const size_t _stringLength = ( const size_t )_callbackArguments[ 1 ];

    l_returnCode = log_query( _string, _stringLength );

    return ( l_returnCode );
}

int16_t __declspec( dllexport ) log$transaction$commit(
    void** _callbackArguments ) {
    uint16_t l_returnCode = 0;

    l_returnCode = log_commit();

    return ( l_returnCode );
}

int16_t __declspec( dllexport ) mainLoop$end( void** _callbackArguments ) {
    uint16_t l_returnCode = 0;

    l_returnCode = log$transaction$commit( NULL );

    return ( l_returnCode );
}
