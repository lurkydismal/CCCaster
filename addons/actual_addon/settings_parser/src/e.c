#include <stdio.h>
#include <string.h>
#if 1
#include <mimalloc.h>
#endif
#if 0
#include <stdlib.h>
#endif
#include <omp.h>

#if 0
#define ARRAY_LENGTH 33554432
#endif

#if 0
#define ARRAY_LENGTH 10000000
#endif

#define ARRAY_LENGTH 2000500000

int main( void ) {
    printf( "being\n" );

    char** l_array = (char**)mi_malloc( ARRAY_LENGTH * sizeof( char* ) );
    ( *(size_t*)( &( l_array[ 0 ] ) ) ) = ARRAY_LENGTH;

    printf( "s: %lu\n", (size_t)( l_array[ 0 ] ) );

    char temp[] = "teasdasd";
    const unsigned int tempLength = strlen( temp );

#pragma omp parallel
    {
        printf( "Hello from process: %d\n", omp_get_thread_num() );
    }

#pragma omp parallel for
    for ( size_t i = 1; i < (size_t)( l_array[ 0 ] ); i++ ) {
        l_array[ i ] = (char*)mi_malloc( ( tempLength + 1 ) * sizeof( char ) );

        char* l_buffer = (char*)mi_malloc( tempLength );

#pragma omp simd
        for ( size_t j = 0; j < tempLength; j++ ) {
            l_buffer[ j ] = ( temp[ j ] + i );
        }

        memcpy( l_array[ i ], l_buffer, tempLength );
        l_array[ i ][ tempLength ] = '\0';

        mi_free( l_buffer );
    }


#if 0
    for ( int i = 1; i < (size_t)( l_array[ 0 ] ); i++ ) {
        printf( "%s : %d\n", l_array[ i ], i );
    }
#endif

    printf( "%lu\n", (size_t)( l_array[ 0 ] ) );

    ( *( size_t* )( &( l_array[ 0 ] ) ) )--;

    {
        const int index = 4;

        mi_free( l_array[ index ] );

        size_t i;

        for ( i = index; i < (size_t)( l_array[ 0 ] ); i++ ) {
            l_array[ i ] = l_array[ i + 1 ];
        }
    }

    printf( "%lu\n", (size_t)( l_array[ 0 ] ) );

#if 0
    for ( int i = 1; i < (size_t)( l_array[ 0 ] ); i++ ) {
        printf( "%s : %d\n", l_array[ i ], i );
    }
#endif

#pragma omp parallel for
    for ( size_t i = 1; i < (size_t)( l_array[ 0 ] ); i++ ) {
        mi_free( l_array [ i ] );
    }

    mi_free( l_array );

    printf( "end\n" );

    return ( 0 );
}
