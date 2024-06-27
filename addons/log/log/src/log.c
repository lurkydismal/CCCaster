#include "log.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static char* g_transactionString;
static size_t g_transactionStringLength = 0;

uint16_t log_query( const char* _string, const size_t _stringLength ) {
    uint16_t l_returnValue = 0;

    //  if ( !g_transactionStringLength ) {
    //      g_transactionString = ( char* )malloc( _stringLength );
    //  }

    //  g_transactionStringLength += _stringLength;

    //  strncat( g_transactionString, _string, _stringLength );

    if ( !g_transactionStringLength ) {
        g_transactionStringLength = _stringLength;

        g_transactionString = ( char* )malloc( g_transactionStringLength );

        strcpy( g_transactionString, _string );

    } else {
        g_transactionStringLength += _stringLength;

        char* l_buffer = ( char* )malloc( g_transactionStringLength );

        strcpy( l_buffer, g_transactionString );
        strcat( l_buffer, _string );

        free( g_transactionString );
        g_transactionString = ( char* )malloc( g_transactionStringLength );

        strcpy( g_transactionString, l_buffer );

        free( l_buffer );
    }

    return ( l_returnValue );
}

uint16_t log_commit( void ) {
    uint16_t l_returnValue = 0;

    if ( g_transactionString ) {
        size_t l_writtenCount =
            fwrite( g_transactionString, sizeof( g_transactionString[ 0 ] ),
                    g_transactionStringLength, g_fileDescriptor );

        if ( l_writtenCount < g_transactionStringLength ) {
            l_returnValue = 1;

            goto EXIT;
        }

        free( g_transactionString );
        g_transactionStringLength = 0;
    }

EXIT:
    return ( l_returnValue );
}
