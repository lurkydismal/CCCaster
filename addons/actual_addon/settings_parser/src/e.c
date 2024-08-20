#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <omp.h>

#if 0
#define ARRAY_LENGTH 33554432
#endif

#define ARRAY_LENGTH 10000000

int main( void ) {
    printf( "being\n" );

    char** l_array = (char**)malloc( ARRAY_LENGTH * sizeof( char* ) );
    ( *(size_t*)( &( l_array[ 0 ] ) ) ) = ARRAY_LENGTH;

    printf( "s: %d\n", l_array[ 0 ] );

    char temp[] = "teasdasd";
    const unsigned int tempLength = strlen( temp );

    for ( int i = 1; i < (size_t)( l_array[ 0 ] ); i++ ) {
        l_array[ i ] = (char*)malloc( ( tempLength + 1 ) * sizeof( char ) );

        memcpy( l_array[ i ], temp, tempLength );
        l_array[ i ][ tempLength ] = '\0';
    }

    for ( int i = 1; i < (size_t)( l_array[ 0 ] ); i++ ) {
        free( l_array [ i ] );
    }

    free( l_array );

    printf( "end\n" );

    return ( 0 );
}
