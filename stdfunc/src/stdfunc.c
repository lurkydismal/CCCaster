#include "stdfunc.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

size_t lengthOfSize( size_t _number ) {
    size_t l_length = 0;

#pragma GCC ivdep
    do {
        l_length++;
        _number /= 10;
    } while ( _number != 0 );

    return ( l_length );
}

char* stoa( size_t _number ) {
#define BUFSIZE ( sizeof( size_t ) * 8 + 1 )

    const size_t l_lengthOfNumber = lengthOfSize( _number );
    char* l_buffer = ( char* )malloc( l_lengthOfNumber + 1 );
    char *l_tail, *l_head = l_buffer, l_buf[ BUFSIZE ];

    l_tail = &( l_buf[ BUFSIZE - 1 ] );
    *l_tail-- = '\0';

    register uint32_t l_characterIndex;

#pragma GCC ivdep
    for ( l_characterIndex = 1; _number != 0; _number /= 10 ) {
        ++l_characterIndex;

        *l_tail-- = ( char )( ( _number % 10 ) + '0' );
    }

    memcpy( l_head, ++l_tail, l_characterIndex );

    return ( l_buffer );

#undef BUFSIZE
}

void** createArray( const size_t _elementSize ) {
    void** l_array = ( void** )malloc( 1 * _elementSize );

    ( *( size_t* )( &( l_array[ 0 ] ) ) ) = ( size_t )( char )( 1 );

    return ( l_array );
}

void insertIntoArray( void*** _array,
                             void* _value,
                             const size_t _elementSize ) {
    const size_t l_arrayLength = ( size_t )( ( *_array )[ 0 ] );

    *_array =
        ( void** )realloc( *_array, ( l_arrayLength + 1 ) * _elementSize );

    ( *_array )[ l_arrayLength ] = _value;

    ( *( size_t* )( &( ( *_array )[ 0 ] ) ) )++;
}

size_t findInArray( const void** _array, const size_t _arrayLength, const void* _value, const size_t _valueBytesCount ) {
    printf( "findInArray: %p %lu %p %lu\n", _array, _arrayLength, _value, _valueBytesCount );
    printf( "findInArray: %lu\n", _value );

    for ( size_t _index = 0; _index < _arrayLength; _index++ ) {
        printf( "findInArray: %lu\n", _index );
        printf( "findInArray: %p\n", _array[ _index ] );
        printf( "findInArray: %lu\n", _array[ _index ] );

        if ( memcmp( _array[ _index ], _value, _valueBytesCount ) == 0 ) {
            return ( _index );
        }
    }

    return ( UINT32_MAX );
}

bool contains( const void** _array, const size_t _arrayLength, const void* _value, const size_t _valueBytesCount ) {
    printf( "contains: %p %lu %p %lu\n", _array, _arrayLength, _value, _valueBytesCount );

    return ( findInArray( _array, _arrayLength, _value, _valueBytesCount ) != UINT32_MAX );
}
