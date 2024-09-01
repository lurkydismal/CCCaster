#include <omp.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <x86intrin.h>

void test( void ) {
    printf( "TEst\n" );

    const int16_t l_directions[][ 2 ] = {
        { -1, -1 }, // 1
        { 0, -1 },  // 2
        { 1, -1 },  // 3
        { -1, 0 },  // 4
        { 0, 0 },   // 5
        { 1, 0 },   // 6
        { -1, 1 },  // 7
        { 0, 1 },   // 8
        { 1, 1 }    // 9
    };

    const uint16_t _pressedButtons[] = { 2, 8, 6 };
    const uint32_t _pressedButtonsLength =
        ( sizeof( _pressedButtons ) / sizeof( _pressedButtons[ 0 ] ) );
    int16_t l_pressedButtons[ 2 ] = { 0, 0 };

    for ( uint32_t _index = 0; _index < _pressedButtonsLength; _index++ ) {
        const int16_t* l_direction =
            l_directions[ _pressedButtons[ _index ] - 1 ];
        l_pressedButtons[ 0 ] += l_direction[ 0 ];
        l_pressedButtons[ 1 ] += l_direction[ 1 ];
    }

    printf( "lpb: %d %d\n", l_pressedButtons[ 0 ], l_pressedButtons[ 1 ] );
}

int main( int _argc, char* argv[] ) {
    printf( "begin\n" );

    test();

    printf( "end\n" );

    return ( 0 );
}
