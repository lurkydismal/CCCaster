#include <omp.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "overlay.h"
#include "stdfunc.h"

void test( void ) {
    char** x = ( char** )createArray( sizeof( char* ) );
    insertIntoArray( ( void*** )&x, "text" );
    insertIntoArray( ( void*** )&x, "rectangle" );
    insertIntoArray( ( void*** )&x, "text" );

    overlayRegister(
        "keyboard", ( const char* const* )x,
        "[text]\ncontent=huh\n[rectangle]\ny=1\n[text]\ncontent=huh", NULL,
        "F4" );

    free( x );
}

int main( int _argc, char* argv[] ) {
    printf( "begin\n" );

    test();

    printf( "end\n" );

    return ( 0 );
}
