#pragma once

#include <stdint.h>

#define MAX_LINE_LENGTH 100

typedef enum { KEY, VALUE } content_t;

uint16_t readSettingsFromFile( const char* _fileName );
uint16_t writeSettingsToFile( const char* _fileName );
uint16_t freeSettingsTable( void );
char*** getContentByLabel( const char* _label );
uint16_t changeSettingsKeyByLabel( const char* _key,
                                   const char* _label,
                                   const char* _value );
