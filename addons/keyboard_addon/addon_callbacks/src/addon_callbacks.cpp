#define WIN32_LEAN_AND_MEAN
#include "addon_callbacks.hpp"

#include <windows.h>

#include <cstdint>

#include "button_input.h"
#include "button_mappings.hpp"
#include "direction_mappings.hpp"

#pragma comment( lib, "user32.lib" )

extern "C" uint16_t __declspec( dllexport )
    mainLoop$getLocalInput( void** _callbackArguments ) {
    uint16_t l_returnValue = 0;
    // player_t l_localPlayer = ( player_t )_callbackArguments[ 0 ];
    direction_t* l_direction = ( direction_t* )_callbackArguments[ 1 ];
    button_t* l_buttons = ( button_t* )_callbackArguments[ 2 ];

    if ( GetAsyncKeyState( g_directionMappings.at( UP ) ) & 0x8000 ) {
        *l_direction = UP;
    }

    if ( GetAsyncKeyState( g_directionMappings.at( DOWN ) ) & 0x8000 ) {
        *l_direction = DOWN;
    }

    if ( GetAsyncKeyState( g_directionMappings.at( LEFT ) ) & 0x8000 ) {
        *l_direction =
            ( *l_direction )
                ? static_cast< direction_t >(
                      --*reinterpret_cast< uint8_t* >( l_direction ) )
                : LEFT;
    }

    if ( GetAsyncKeyState( g_directionMappings.at( RIGHT ) ) & 0x8000 ) {
        *l_direction =
            ( *l_direction )
                ? static_cast< direction_t >(
                      ++*reinterpret_cast< uint8_t* >( l_direction ) )
                : RIGHT;
    }

    for ( const std::pair< char, button_t > _buttonMapping :
          g_buttonMappings ) {
        // 0x8000 is 0b1000_0000_0000_0000
        // If the highest bit is ON, then it is hold
        const bool l_isHeld =
            ( GetAsyncKeyState( _buttonMapping.first ) & 0x8000 );

        if ( l_isHeld ) {
            *l_buttons = static_cast< button_t >(
                static_cast< uint16_t >( *l_buttons ) |
                static_cast< uint16_t >( _buttonMapping.second ) );
        }
    }

    {
        // 6 - 0110
        // 4 - 0100
        // 2 - 0010
        if ( ( *l_direction == 0 ) || ( *l_direction & 0b1001 ) ) {
            *l_buttons =
                static_cast< button_t >( static_cast< uint16_t >( *l_buttons ) &
                                         ~static_cast< uint16_t >( AB ) );
        }
    }

    return ( l_returnValue );
}
