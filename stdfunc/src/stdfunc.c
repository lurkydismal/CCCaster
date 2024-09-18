#include "stdfunc.h"

#include <omp.h>
#include <string.h>

#define arrayLengthPointer( _array ) ( ( size_t* )( &( _array[ 0 ] ) ) )

#ifdef __cplusplus

extern "C" {

#endif

size_t lengthOfNumber( size_t _number ) {
    if ( _number == 0 ) {
        return ( 1 );
    }

    size_t l_length = 0;

    do {
        l_length++;
        _number /= 10;
    } while ( _number );

    return ( l_length );
}

int64_t power( int64_t _base, uint8_t _exponent ) {
    const uint8_t l_highestBitSet[] = {
        0,   1,   2,   2,   3,   3,   3,   3,   4,   4,   4,   4,   4,   4,
        4,   4,   5,   5,   5,   5,   5,   5,   5,   5,   5,   5,   5,   5,
        5,   5,   5,   5,   6,   6,   6,   6,   6,   6,   6,   6,   6,   6,
        6,   6,   6,   6,   6,   6,   6,   6,   6,   6,   6,   6,   6,   6,
        6,   6,   6,   6,   6,   6,   6,   255, // anything past 63 is a
                                                // guaranteed overflow with (
                                                // _base > 1 )
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    };

    int64_t result = 1;

    switch ( l_highestBitSet[ _exponent ] ) {
        case 255: // we use 255 as an overflow marker and return 0 on
                  // overflow/underflow
            if ( _base == 1 ) {
                return ( 1 );
            }

            if ( _base == -1 ) {
                return ( 1 - 2 * ( _exponent & 1 ) );
            }

            return ( 0 );

        case 6: {
            if ( _exponent & 1 ) {
                result *= _base;
            }

            _exponent >>= 1;
            _base *= _base;
        }

        case 5: {
            if ( _exponent & 1 ) {
                result *= _base;
            }

            _exponent >>= 1;
            _base *= _base;
        }

        case 4: {
            if ( _exponent & 1 ) {
                result *= _base;
            }

            _exponent >>= 1;
            _base *= _base;
        }

        case 3: {
            if ( _exponent & 1 ) {
                result *= _base;
            }

            _exponent >>= 1;
            _base *= _base;
        }

        case 2: {
            if ( _exponent & 1 ) {
                result *= _base;
            }

            _exponent >>= 1;
            _base *= _base;
        }

        case 1: {
            if ( _exponent & 1 ) {
                result *= _base;
            }
        }

        default: {
            return ( result );
        }
    }
}

char* stoa( size_t _number ) {
    const size_t l_lengthOfNumber = lengthOfNumber( _number );
    char* l_buffer = ( char* )malloc( l_lengthOfNumber + 1 );

#pragma omp simd
    for ( ssize_t _characterIndex = ( l_lengthOfNumber - 1 );
          _characterIndex >= 0; _characterIndex-- ) {
        l_buffer[ _characterIndex ] = ( char )( ( ( _number / ( power( 10, ( l_lengthOfNumber - _characterIndex - 1 ) ) ) ) % 10 ) + '0' );
    }

    l_buffer[ l_lengthOfNumber ] = '\0';

    return ( l_buffer );
}

void** createArray( const size_t _elementSize ) {
    void** l_array = ( void** )malloc( 1 * _elementSize );

    *arrayLengthPointer( l_array ) = ( size_t )( char )( 1 );

    return ( l_array );
}

void preallocateArray( void*** _array,
                       const size_t _length ) {
    const size_t l_currentArrayLength = arrayLength( *_array );

    *_array = ( void** )realloc( *_array, ( ( l_currentArrayLength + _length + 1 ) * sizeof( ( *_array )[ 0 ] ) ) );

    *arrayLengthPointer( *_array ) = ( size_t )( char )( l_currentArrayLength + _length + 1 );
}

void insertIntoArray( void*** _array,
                      void* _value ) {
    const size_t l_arrayLength = arrayLength( *_array );

    *_array =
        ( void** )realloc( *_array, ( 1 + l_arrayLength + 1 ) * sizeof( ( *_array )[ 0 ] ) );

    ( *_array )[ l_arrayLength + 1 ] = _value;

    ( *arrayLengthPointer( *_array ) )++;
}

void insertIntoArrayByIndex( void*** _array,
                             const size_t _index,
                             void* _value ) {
    ( *_array )[ _index ] = _value;
}

ssize_t findStringInArray( const char** _array,
                           const size_t _arrayLength,
                           const char* _value ) {
    ssize_t l_index = -1;

#pragma omp simd
    for ( size_t _index = 0; _index < _arrayLength; _index++ ) {
        if ( strcmp( _array[ _index ], _value ) == 0 ) {
            l_index = _index;
        }
    }

    return ( l_index );
}

ssize_t findInArray( const size_t* _array,
                     const size_t _arrayLength,
                     const size_t _value ) {
    ssize_t l_index = -1;

#pragma omp simd
    for ( size_t _index = 0; _index < _arrayLength; _index++ ) {
        if ( _array[ _index ] == _value ) {
            l_index = _index;
        }
    }

    return ( l_index );
}

bool containsString( const char** _array,
                     const size_t _arrayLength,
                     const char* _value ) {
    return ( findStringInArray( _array, _arrayLength, _value ) >= 0 );
}

bool contains( const size_t* _array,
               const size_t _arrayLength,
               const size_t _value ) {
    return ( findInArray( _array, _arrayLength, _value ) >= 0 );
}

#ifdef __cplusplus
}

#endif
