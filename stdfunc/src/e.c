#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "stdfunc.h"

void test( void ) {
    fdatasync( 1 );

#pragma omp parallel for
    for ( volatile size_t _index = 0; _index < 1234567890; _index++ ) {
        char* l_x = stoa( _index );

        if ( ( _index % 10000000 ) == 0 ) {
            write( 1, l_x, ( lengthOfNumber( _index ) + 1 ) );
            write( 1, "\n", 2 );
        }

        free( l_x );
    }
}

int main( int _argumentCount, char* _argumentArray[] ) {
    printf( "begin\n" );

    test();

    printf( "end\n" );

    return ( 0 );
}
