#include "log.h"

#include <string.h>

static char g_transactionString[ 1024 * 2 ];
static size_t g_transactionStringLength = 0;

uint16_t log_query( const char* _string, const size_t _stringLength ) {
    uint16_t l_returnValue = 0;

    memcpy( g_transactionString + g_transactionStringLength - 1, _string,
            _stringLength );
    g_transactionStringLength += _stringLength;

    return ( l_returnValue );
}

uint16_t log_commit( void ) {
    uint16_t l_returnValue = 0;

    if ( g_transactionStringLength ) {
        size_t l_writtenCount =
            fwrite( g_transactionString, sizeof( g_transactionString[ 0 ] ),
                    g_transactionStringLength, g_fileDescriptor );

        if ( l_writtenCount < g_transactionStringLength ) {
            l_returnValue = 1;

            goto EXIT;
        }

        fflush( g_fileDescriptor );

        g_transactionStringLength = 0;
    }

EXIT:
    return ( l_returnValue );
}
