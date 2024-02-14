#pragma once

#include <cstdint>
#include <string>
#include <map>

extern std::map< std::string, char > g_buttonMappings;

bool setButtonMappings( std::map< std::string, char > _buttonMappings );
