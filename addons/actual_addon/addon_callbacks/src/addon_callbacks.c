#include <stdlib.h>

#include "_useCallback.h"
#include "button_t.h"
#include "direction_t.h"
#include "player_t.h"
#include "settings_parser.h"

useCallbackFunction_t g_useCallback = NULL;

uint16_t __declspec( dllexport ) IDirect3D9Ex$CreateDevice(
    void** _callbackArguments ) {
    _useCallbackInitialize();

    return ( 0 );
}

uint16_t __declspec( dllexport ) game$applyInput( void** _callbackArguments ) {
    int16_t l_returnValue = 0;

    button_t* _buttons = ( button_t* )_callbackArguments[ 0 ];
    direction_t* _direction = ( direction_t* )_callbackArguments[ 1 ];
    player_t* _player = ( player_t* )_callbackArguments[ 2 ];

    char* const l_inputsStructureAddress = *( ( char** )( 0x76E6AC ) );
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

uint16_t __declspec( dllexport ) core$readSettingsFromString(
    void** _callbackArguments ) {
    int16_t l_returnValue = 0;

    const char* _string = ( const char* )_callbackArguments[ 0 ];

    l_returnValue = readSettingsFromString( _string );

    if ( l_returnValue == 0 ) {
        writeSettingsToFile( SETTINGS_FILE_PATH );
    }

    return ( l_returnValue );
}

uint16_t __declspec( dllexport ) core$getSettingsContentByLabel(
    void** _callbackArguments ) {
    int16_t l_returnValue = 0;

    char**** _returnValue = ( char**** )_callbackArguments[ 0 ];
    const char* _label = ( const char* )_callbackArguments[ 1 ];

    *_returnValue = getSettingsContentByLabel( _label );

    l_returnValue = ( *_returnValue == NULL );

    return ( l_returnValue );
}

uint16_t __declspec( dllexport ) core$changeSettingsKeyByLabel(
    void** _callbackArguments ) {
    int16_t l_returnValue = 0;

    const char* const* _key = ( const char* const* )_callbackArguments[ 0 ];
    const char* const* _label = ( const char* const* )_callbackArguments[ 1 ];
    const char* const* _value = ( const char* const* )_callbackArguments[ 2 ];

    l_returnValue = changeSettingsKeyByLabel( *_key, *_label, *_value );

    if ( l_returnValue == 0 ) {
        writeSettingsToFile( SETTINGS_FILE_PATH );
    }

    return ( l_returnValue );
}
