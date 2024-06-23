#include <stdio.h>
#include <stdlib.h>

char* readFile( const char* _fileName ) {
    char* l_string = NULL;
    size_t l_stringSize;
    size_t l_readSize;
    FILE* l_fileHandle = fopen( _fileName, "r" );

    if ( l_fileHandle ) {
        fseek( l_fileHandle, 0, SEEK_END );
        // Offset from the first to the last byte, or in other words, filesize
        l_stringSize = ftell( l_fileHandle );
        // go back to the start of the file
        rewind( l_fileHandle );

        l_string = ( char* )malloc( sizeof( char ) * ( l_stringSize + 1 ) );

        // Read it all in one operation
        l_readSize =
            fread( l_string, sizeof( char ), l_stringSize, l_fileHandle );

        // fread doesn't set it so put a \0 in the last position
        l_string[ l_stringSize ] = '\0';

        if ( l_stringSize != l_readSize ) {
            free( l_string );

            l_string = NULL;
        }

        fclose( l_fileHandle );
    }

    return ( l_string );
}

void writeFile( const char* _fileName, const char* _string ) {
    FILE* l_fileHandle = fopen( _fileName, "wb" );

    if ( l_fileHandle ) {
        fprintf( l_fileHandle, "%s\n", _string );
    }
}
