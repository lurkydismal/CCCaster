#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define _findInArray( _array, _value ) findInArray( _array, (size_t)( _array[ 0 ] ), _value )
#define _contains( _array, _value ) contains( _array, (size_t)( _array[ 0 ] ), _value )

size_t lengthOfSize( size_t _number );
char* stoa( size_t _number );
void** createArray( const size_t _elementSize );
void insertIntoArray( void*** _array, void* _value, const size_t _elementSize );
ssize_t findStringInArray( const char** _array, const char* _value );
ssize_t findInArray( const size_t* _array, const size_t _arrayLength, const size_t _value );
bool containsString( const char** _array, const char* _value );
bool contains( const size_t* _array, const size_t _arrayLength, const size_t _value );
