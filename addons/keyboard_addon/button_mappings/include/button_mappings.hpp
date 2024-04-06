#pragma once

#include <cstdint>
#include <vector>

#include "button_input.h"

extern std::vector< std::pair< char, button_t > > g_buttonMappings;

bool setButtonMappings(
    std::vector< std::pair< char, button_t > > _buttonMappings );
