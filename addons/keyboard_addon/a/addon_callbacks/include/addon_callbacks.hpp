#pragma once

#include <cstdint>
#include <vector>

extern std::vector< std::pair< char, uint16_t > > g_buttonMappings;

extern "C" {

uint16_t __declspec( dllexport )
    mainLoop$getLocalInput( void** _callbackArguments );

}
