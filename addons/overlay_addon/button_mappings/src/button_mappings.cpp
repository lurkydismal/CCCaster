#include "button_mappings.hpp"

std::map< std::string, char > g_buttonMappings;

bool setButtonMappings( std::map< std::string, char > _buttonMappings ) {
    bool l_returnValue = true;

    g_buttonMappings = _buttonMappings;

    return ( l_returnValue );
}
