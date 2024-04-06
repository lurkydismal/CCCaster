#pragma once

#include <cstdint>
#include <map>
#include <string>

extern std::map< std::string, char > g_hotkeyMappings;

bool setHotkeyMappings( std::map< std::string, char > _hotkeyMappings );
