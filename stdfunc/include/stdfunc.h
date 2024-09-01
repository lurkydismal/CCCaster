#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define arrayLength( _array ) ( (size_t)( _array[ 0 ] ) - 1 )
#define arrayFirstElement( _array ) ( _array + 1 )
#define _findStringInArray( _array, _value ) return ( findStringInArrayInArray( (const char**)( arrayFirstElement( _array ) ), arrayLength( _array ), _value ) + 1 )
#define _findArrayInArray( _array, _value ) return ( findArrayInArrayInArray( (const char**)( arrayFirstElement( _array ) ), arrayLength( _array ), _value, ( sizeof( _value ) / sizeof( _value[ 0 ] ) ) ) + 1 )
#define _findInArray( _array, _value ) findInArray( arrayFirstElement( _array ), arrayLength( _array ), _value )
#define _containsString( _array, _value ) containsString( (const char**)( arrayFirstElement( _array ) ), arrayLength( _array ), _value )
#define _containsArray( _array, _value ) containsArray( (const char**)( arrayFirstElement( _array ) ), arrayLength( _array ), _value, ( sizeof( _value ) / sizeof( _value[ 0 ] ) ) )
#define _contains( _array, _value ) contains(  arrayFirstElement( _array ), arrayLength( _array ), _value )

size_t lengthOfSize( size_t _number );
char* stoa( size_t _number );
void** createArray( const size_t _elementSize );
void insertIntoArray( void*** _array, void* _value, const size_t _elementSize );
ssize_t findStringInArray( const char** _array, const size_t _arrayLength, const char* _value );
ssize_t findArrayInArray( const char** _array, const size_t _arrayLength, const char* _value, const size_t _valueLength );
ssize_t findInArray( const size_t* _array, const size_t _arrayLength, const size_t _value );
bool containsString( const char** _array, const size_t _arrayLength, const char* _value );
bool containsArray( const char** _array, const size_t _arrayLength, const char* _value, const size_t _valueLength );
bool contains( const size_t* _array, const size_t _arrayLength, const size_t _value );
