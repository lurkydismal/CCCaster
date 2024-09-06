#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define arrayLength( _array ) ( ( size_t )( _array[ 0 ] ) - 1 )
#define arrayFirstElement( _array ) ( _array + 1 )

#define _findStringInArray( _array, _value )                      \
    ( findStringInArrayInArray(                            \
                                                           ( const char** )( arrayFirstElement( _array ) ), \
                                                           arrayLength( _array ), _value ) +                \
                                                           1 )
#define _findInArray( _array, _value ) \
    findInArray( arrayFirstElement( _array ), arrayLength( _array ), _value )

#define _containsString( _array, _value )                            \
    containsString( ( const char** )( arrayFirstElement( _array ) ), \
                    arrayLength( _array ), _value )
#define _contains( _array, _value ) \
    contains( arrayFirstElement( _array ), arrayLength( _array ), _value )

#ifdef __cplusplus

extern "C" {

#endif

size_t lengthOfSize( size_t _number );
char* stoa( size_t _number );
void** createArray( const size_t _elementSize );
void preallocateArray( void*** _array, const size_t _size );
void insertIntoArray( void*** _array, void* _value, const size_t _elementSize );
void insertIntoArrayByIndex( void*** _array, const size_t _index, void* _value, const size_t _elementSize );
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
