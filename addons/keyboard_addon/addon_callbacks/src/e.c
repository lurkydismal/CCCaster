#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <omp.h>

void test( void ) {
    printf( "TEst\n" );
}

int main( int _argc, char* argv[] ) {
    printf( "begin\n" );

    test();

    printf( "end\n" );

    return ( 0 );
}
