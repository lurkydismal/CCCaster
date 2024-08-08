#pragma once

#include <stdint.h>

#define MAX_LINE_LENGTH 100

uint16_t readSettingsFromFile( const char* _fileName );
uint16_t readSettingsFromString( const char* _text, const size_t _textLength );
uint16_t writeSettingsToFile( const char* _fileName );
uint16_t freeSettingsTable( void );
const char* const* const* getSettingsContentByLabel( const char* _label );
uint16_t changeSettingsKeyByLabel( const char* _key,
                                   const char* _label,
                                   const char* _value );
