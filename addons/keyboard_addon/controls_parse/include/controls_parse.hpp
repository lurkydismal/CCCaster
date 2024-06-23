#pragma once

#include "nlohmann/json.hpp"

#define CONTROLS_PREFERENCES_FILE_NAME "controls"
#define CONTROLS_PREFERENCES_FILE_EXTENSION "json"
#define BACKUP_ADDITION_TO_FILE_NAME "_backup"

using json = nlohmann::ordered_json;

extern json g_jsonControlsKeyboard;

extern "C" {

char* readFile( const char* _fileName );

void writeFile( const char* _fileName, const char* _string );
}
