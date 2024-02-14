#pragma once

#include <cstdint>
#include <map>

#include "direction_input.h"

extern std::map< direction_t, char > g_directionMappings;

bool setDirectionMappings( std::map< direction_t, char > _directionMappings );
