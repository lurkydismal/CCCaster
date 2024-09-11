#include "log.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

static char g_transactionString[ MAX_TRANSACITON_STRING_SIZE ];
static size_t g_transactionStringLength = 0;

uint16_t log_query( const char* _string ) {
    uint16_t l_returnValue = 0;

    const size_t l_stringLength = strlen( _string );
    memcpy( ( g_transactionString + g_transactionStringLength ), _string,
            l_stringLength + 1 );
    g_transactionStringLength += l_stringLength;

    return ( l_returnValue );
}

uint16_t log_commit( void ) {
    uint16_t l_returnValue = 1;

    if ( g_transactionStringLength ) {
        size_t l_writtenCount =
            fwrite( g_transactionString, sizeof( g_transactionString[ 0 ] ),
                    g_transactionStringLength, g_fileDescriptor );

        l_returnValue = !( l_writtenCount == g_transactionStringLength );

        fflush( g_fileDescriptor );

        if ( l_returnValue != 0 ) {
            goto EXIT;
        }

#if defined( PRINT_LOG )
        {
            const char l_logSignature[] = { 'L', 'O', 'G', ':' };
            const size_t l_logSignatureLength = sizeof( l_logSignature );
            const size_t l_bufferLength =
                ( g_transactionStringLength + l_logSignatureLength );
            char* l_buffer =
                ( char* )malloc( ( l_bufferLength + 1 ) * sizeof( char ) );

            memcpy( l_buffer, l_logSignature, l_logSignatureLength );
            memcpy( ( l_buffer + l_logSignatureLength ), g_transactionString,
                    ( g_transactionStringLength + 1 ) );

            l_writtenCount = fwrite( l_buffer, sizeof( l_buffer[ 0 ] ),
                                     l_bufferLength, stdout );

            free( l_buffer );

            l_returnValue =
                !( l_writtenCount == ( g_transactionStringLength + 1 ) );

            fflush( stdout );
        }
#endif

    EXIT:
        g_transactionStringLength = 0;
    }

    return ( l_returnValue );
}
