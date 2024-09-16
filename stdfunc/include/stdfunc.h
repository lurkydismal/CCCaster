#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#define arrayLength( _array ) ( ( size_t )( _array[ 0 ] ) - 1 )
#define arrayFirstElementPointer( _array ) ( _array + 1 )

#define _findStringInArray( _array, _value )                      \
    ( findStringInArrayInArray(                                   \
          ( const char** )( arrayFirstElementPointer( _array ) ), \
          arrayLength( _array ), _value ) +                       \
      1 )
#define _findInArray( _array, _value )                                      \
    findInArray( arrayFirstElementPointer( _array ), arrayLength( _array ), \
                 _value )

#define _containsString( _array, _value )                                   \
    containsString( ( const char** )( arrayFirstElementPointer( _array ) ), \
                    arrayLength( _array ), _value )
#define _contains( _array, _value )                                      \
    contains( arrayFirstElementPointer( _array ), arrayLength( _array ), \
              _value )

#ifdef __cplusplus

extern "C" {

#endif

size_t lengthOfNumber( size_t _number );
int64_t power( int64_t _base, uint8_t _exponent );
char* stoa( size_t _number );
void** createArray( const size_t _elementSize );
void preallocateArray( void*** _array,
                       const size_t _length,
                       const size_t _elementSize );
void insertIntoArray( void*** _array, void* _value, const size_t _elementSize );
void insertIntoArrayByIndex( void*** _array,
                             const size_t _index,
                             void* _value );
ssize_t findStringInArray( const char** _array,
                           const size_t _arrayLength,
                           const char* _value );
ssize_t findInArray( const size_t* _array,
                     const size_t _arrayLength,
                     const size_t _value );
bool containsString( const char** _array,
                     const size_t _arrayLength,
                     const char* _value );
bool contains( const size_t* _array,
               const size_t _arrayLength,
               const size_t _value );

#ifdef __cplusplus
}

#endif
