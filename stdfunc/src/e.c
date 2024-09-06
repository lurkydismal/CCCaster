#include <stdio.h>
#include <stdlib.h>

int main( void ) {
    printf( "begin\n" );

    size_t* tt = ( size_t* )malloc( 5 * sizeof( size_t ) );

    size_t t = 5;
    tt[ 0 ] = t;
    size_t t2 = 1;
    tt[ 1 ] = t2;
    size_t t3 = 2;
    tt[ 2 ] = t3;
    size_t t4 = 3;
    tt[ 3 ] = t4;
    size_t t5 = 4;
    tt[ 4 ] = t5;

    size_t* ttt = tt + 4;
    printf( "%lu\n", ttt[ 0 ] );

    free( tt );

    printf( "end\n" );

    return ( 0 );
}
