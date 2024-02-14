#include "direction_mappings.hpp"

std::map< direction_t, char > g_directionMappings;

bool setDirectionMappings( std::map< direction_t, char > _directionMappings ) {
    bool l_returnValue = true;

    g_directionMappings = _directionMappings;

    return ( l_returnValue );
}
