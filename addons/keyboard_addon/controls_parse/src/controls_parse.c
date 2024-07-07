#include <stdio.h>
#include <stdlib.h>

char* readFile( const char* _fileName ) {
    char* l_string = NULL;
    size_t l_stringLength;
    size_t l_readLength;

    FILE* l_fileHandle = fopen( _fileName, "r" );

    if ( l_fileHandle ) {
        fseek( l_fileHandle, 0, SEEK_END );

        // Offset from the first to the last byte, or in other words, filesize
        l_stringLength = ftell( l_fileHandle );

        // Go back to the start of the file
        rewind( l_fileHandle );

        l_string = ( char* )malloc( sizeof( char ) * ( l_stringLength + 1 ) );

        // Read it all in one operation
        l_readLength =
            fread( l_string, sizeof( char ), l_stringLength, l_fileHandle );

        // NUL in the last position
        l_string[ l_stringLength ] = '\0';

        if ( l_stringLength != l_readLength ) {
            free( l_string );

            l_string = NULL;
        }

        fclose( l_fileHandle );
    }

    return ( l_string );
}

void writeFile( const char* _fileName,
                const char* _string,
                const size_t _stringLength ) {
    FILE* l_fileHandle = fopen( _fileName, "w" );

    if ( l_fileHandle ) {
        fwrite( _string, sizeof( _string[ 0 ] ), _stringLength, l_fileHandle );
    }

    fclose( l_fileHandle );
}
