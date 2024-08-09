#include "log.h"

#include <stdarg.h>
#include <string.h>

static char g_transactionString[ MAX_TRANSACITON_STRING_SIZE ];
static size_t g_transactionStringLength = 0;

uint16_t log_query( const char* _string ) {
    uint16_t l_returnValue = 0;

    const size_t l_stringLength = strlen( _string );
    memcpy( ( g_transactionString + g_transactionStringLength ), _string,
            l_stringLength );
    g_transactionStringLength += ( l_stringLength - 1 );

    return ( l_returnValue );
}

uint16_t log_commit( void ) {
    uint16_t l_returnValue = 1;

    if ( g_transactionStringLength ) {
        size_t l_writtenCount =
            fwrite( g_transactionString, sizeof( g_transactionString[ 0 ] ),
                    ( g_transactionStringLength + 1 ), g_fileDescriptor );

        l_returnValue =
            !( l_writtenCount == ( g_transactionStringLength + 1 ) );

        printf( "LOG: %s", g_transactionString );

        fflush( g_fileDescriptor );

        g_transactionStringLength = 0;
    }

    return ( l_returnValue );
}
