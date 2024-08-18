#include "stdfunc.h"

#include <stdlib.h>
#include <string.h>

size_t lengthOfSize( size_t _number ) {
    if ( _number == 0 ) {
        return ( 1 );
    }

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

    register uint32_t l_characterIndex = 2;

    if ( l_lengthOfNumber == 1 ) {
        *l_tail-- = (char)( _number + '0' );

        goto EXIT;
    }

    {

#pragma GCC ivdep
        for ( l_characterIndex = 1; _number != 0; _number /= 10 ) {
            ++l_characterIndex;

            *l_tail-- = ( char )( ( _number % 10 ) + '0' );
        }
    }

EXIT:
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

ssize_t findStringInArray( const char** _array, const char* _value ) {
    const size_t l_arrayLength = (size_t)( _array[ 0 ]  );

    for ( size_t _index = 1; _index < l_arrayLength; _index++ ) {
        if ( strcmp(  _array[ _index ], _value ) == 0 ) {
            return ( _index );
        }
    }

    return ( -1 );
}

ssize_t findInArray( const size_t* _array, const size_t _arrayLength, const size_t _value ) {
    for ( size_t _index = 0; _index < _arrayLength; _index++ ) {
        if ( _array[ _index ] == _value ) {
            return ( _index );
        }
    }

    return ( -1 );
}

bool containsString( const char** _array, const char* _value ) {
    return ( findStringInArray( _array, _value ) >= 0 );
}

bool contains( const size_t* _array, const size_t _arrayLength, const size_t _value ) {
    return ( findInArray( _array, _arrayLength, _value ) >= 0 );
}
