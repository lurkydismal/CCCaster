#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

size_t lengthOfSize( size_t _number );
char* stoa( size_t _number );
void** createArray( const size_t _elementSize );
void insertIntoArray( void*** _array,
        void* _value,
        const size_t _elementSize );
size_t findInArray( const void** _array, const size_t _arrayLength, const void* _value, const size_t _valueBytesCount );
bool contains( const void** _array, const size_t _arrayLength, const void* _value, const size_t _valueBytesCount );
