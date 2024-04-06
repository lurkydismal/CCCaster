#include "hotkey_mappings.hpp"

std::map< std::string, char > g_hotkeyMappings;

bool setHotkeyMappings( std::map< std::string, char > _hotkeyMappings ) {
    bool l_returnValue = true;

    g_hotkeyMappings = _hotkeyMappings;

    return ( l_returnValue );
}
