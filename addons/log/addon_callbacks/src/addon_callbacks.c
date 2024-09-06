#include <stdint.h>

#include "log.h"

int16_t __declspec( dllexport ) log$transaction$query(
    void** _callbackArguments ) {
    uint16_t l_returnCode = 0;
    printf( "TEST\n" );
    const char* _string = ( const char* )_callbackArguments[ 0 ];
    printf( "%s\n", _string );

    l_returnCode = log_query( _string );

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
