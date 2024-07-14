#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <stdlib.h>
#include "_useCallback.hpp"
#include "button_t.h"
#include "direction_t.h"
#include "player_t.h"

useCallbackFunction_t g_useCallback = NULL;

uint16_t __declspec( dllexport ) IDirect3D9Ex$CreateDevice(
    void** _callbackArguments ) {
    HMODULE l_statesDll = GetModuleHandleA( "states.dll" );

    if ( !l_statesDll ) {
        exit( 1 );
    }

    g_useCallback =
        ( useCallbackFunction_t )GetProcAddress( l_statesDll, "useCallback" );

    if ( !g_useCallback ) {
        exit( 1 );
    }

    return ( 0 );
}

uint16_t __declspec( dllexport ) game$applyInput(
    void** _callbackArguments ) {
    int16_t l_returnValue = 0;

    button_t* _buttons = ( button_t* )_callbackArguments[ 0 ];
    direction_t* _direction = ( direction_t* )_callbackArguments[ 1 ];
    player_t* _player = ( player_t* )_callbackArguments[ 2 ];

    char* const l_inputsStructureAddress =
        *( ( char** )( 0x76E6AC ) );
    // const uintptr_t l_firstPlayerDirectionOffset = 0x18;  // 24
    // const uintptr_t l_firstPlayerButtonsOffset = 0x24;    // 36
    // const uintptr_t l_secondPlayerDirectionOffset = 0x2C; // 44
    // const uintptr_t l_secondPlayerButtonsOffset = 0x38;   // 56

    *( uint16_t* )( l_inputsStructureAddress + 16 +
                    ( 20 * ( ( uint8_t )*_player ) ) ) =
        ( uint16_t )( *_buttons );
    *( uint16_t* )( l_inputsStructureAddress + 4 +
                    ( 20 * ( ( uint8_t )*_player ) ) ) =
        ( uint16_t )( *_direction );

    return ( l_returnValue );
}
