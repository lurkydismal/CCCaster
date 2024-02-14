#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <cstdint>
#include <fstream>
#include <vector>

#include "a.h"
#include "button_input.h"
#include "addon_callbacks.hpp"

#pragma comment( lib, "user32.lib" )

std::vector< std::pair< char, uint16_t > > g_buttonMappings = {
        { 'Z', A | CONFIRM }, { 'X', B | CANCEL }, { 'C', C },
        { 'V', D },           { 'D', E },          { VK_OEM_4, FN1 },
        { 'R', FN2 },         { 'T', START } };

extern "C" uint16_t __declspec( dllexport )
    mainLoop$getLocalInput( void** _callbackArguments ) {
    uint16_t l_returnValue = 0;
    uint8_t localPlayer = ( uint8_t )_callbackArguments[ 0 ];
    uint16_t* direction = ( uint16_t* )_callbackArguments[ 1 ];
    uint16_t* buttons = ( uint16_t* )_callbackArguments[ 2 ];

    if ( GetAsyncKeyState( VK_UP ) & 0x8000 ) {
        *direction = 8;
    }

    if ( GetAsyncKeyState( VK_DOWN ) & 0x8000 ) {
        *direction = 2;
    }

    if ( GetAsyncKeyState( VK_LEFT ) & 0x8000 ) {
        *direction = ( *direction ) ? --*direction : 4;
    }

    if ( GetAsyncKeyState( VK_RIGHT ) & 0x8000 ) {
        *direction = ( *direction ) ? ++*direction : 6;
    }

    for ( const std::pair< char, uint16_t > _buttonMapping :
          g_buttonMappings ) {
        // 0x8000 is 0b1000_0000_0000_0000
        // If the highest bit is ON, then it is hold
        const bool l_isHeld =
            ( GetAsyncKeyState( _buttonMapping.first ) & 0x8000 );

        if ( l_isHeld ) {
            *buttons |= _buttonMapping.second;
        }
    }

    {
        const bool l_isHeld = ( GetAsyncKeyState( 'S' ) & 0x8000 );

        if ( l_isHeld ) {
            // 6 - 0110
            // 4 - 0100
            // 2 - 0010
            if ( ( *direction != 0 ) && !( *direction & 0b1001 ) ) {
                *buttons |= AB;
            }
        }
    }

    return ( l_returnValue );
}
