#include "button_mappings.hpp"

std::vector< std::pair< char, button_t > > g_buttonMappings;

bool setButtonMappings( std::vector< std::pair< char, button_t > > _buttonMappings ) {
    bool l_returnValue = true;

    g_buttonMappings = _buttonMappings;

    return ( l_returnValue );
}
