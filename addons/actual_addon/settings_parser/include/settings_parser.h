#pragma once

#include <stdint.h>

#define SETTINGS_FILE_NAME "settings"
#define SETTINGS_FILE_EXTENSION "ini"
#define SETTINGS_FILE_PATH SETTINGS_FILE_NAME "." SETTINGS_FILE_EXTENSION
#define MAX_LINE_LENGTH 100

uint16_t readSettingsFromFile( const char* _fileName );
uint16_t readSettingsFromString( const char* _text );
uint16_t writeSettingsToFile( const char* _fileName );
uint16_t freeSettingsTable( void );
const char* const* const* getSettingsContentByLabel( const char* _label );
uint16_t changeSettingsKeyByLabel( const char* _key,
                                   const char* _label,
                                   const char* _value );
