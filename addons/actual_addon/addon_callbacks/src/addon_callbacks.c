#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "_useCallback.h"
#include "button_t.h"
#include "direction_t.h"
#include "player_t.h"
#include "settings_parser.h"
#include "stdfunc.h"

useCallbackFunction_t g_useCallback = NULL;

static void applyInput( button_t _buttons,
                        direction_t _direction,
                        player_t _player ) {
    char* const l_inputsStructureAddress = *( ( char** )( 0x76E6AC ) );
#if 0
    const uintptr_t l_firstPlayerDirectionOffset = 0x18;  // 24
    const uintptr_t l_firstPlayerButtonsOffset = 0x24;    // 36
    const uintptr_t l_secondPlayerDirectionOffset = 0x2C; // 44
    const uintptr_t l_secondPlayerButtonsOffset = 0x38;   // 56
#endif

    *( ( uint16_t* )( l_inputsStructureAddress + 16 +
                      ( 20 * ( ( uint8_t )_player ) ) ) ) =
        ( uint16_t )( _buttons );
    *( ( uint16_t* )( l_inputsStructureAddress + 4 +
                      ( 20 * ( ( uint8_t )_player ) ) ) ) =
        ( uint16_t )( _direction );
}

uint16_t __declspec( dllexport ) IDirect3D9Ex$CreateDevice(
    void** _callbackArguments ) {
    _useCallbackInitialize();

    return ( 0 );
}

uint16_t __declspec( dllexport ) core$readSettingsFromString(
    void** _callbackArguments ) {
    uint16_t l_returnValue = 0;

    const char* _string = ( const char* )_callbackArguments[ 0 ];

    l_returnValue = readSettingsFromString( _string );

    if ( !l_returnValue ) {
        writeSettingsToFile( SETTINGS_FILE_PATH );
    }

    return ( l_returnValue );
}

uint16_t __declspec( dllexport ) core$getSettingsContentByLabel(
    void** _callbackArguments ) {
    uint16_t l_returnValue = 0;

    char**** _returnValue = ( char**** )_callbackArguments[ 0 ];
    const char* _label = ( const char* )_callbackArguments[ 1 ];

    *_returnValue = getSettingsContentByLabel( _label );

    l_returnValue = ( *_returnValue == NULL );

    return ( l_returnValue );
}

uint16_t __declspec( dllexport ) core$changeSettingsKeyByLabel(
    void** _callbackArguments ) {
    uint16_t l_returnValue = 0;

    const char* const* _key = ( const char* const* )_callbackArguments[ 0 ];
    const char* const* _label = ( const char* const* )_callbackArguments[ 1 ];
    const char* const* _value = ( const char* const* )_callbackArguments[ 2 ];

    l_returnValue = changeSettingsKeyByLabel( *_key, *_label, *_value );

    if ( !l_returnValue ) {
        writeSettingsToFile( SETTINGS_FILE_PATH );
    }

    return ( l_returnValue );
}

uint16_t __declspec( dllexport ) game$applyInput( void** _callbackArguments ) {
    uint16_t l_returnValue = 0;

    char*** _activeMappedKeys = ( char*** )_callbackArguments[ 0 ];
    size_t l_activeMappedKeysLength = arrayLength( *_activeMappedKeys );
    direction_t l_direction = NEUTRAL_DIRECTION;
    button_t l_buttons = NEUTRAL_BUTTON;
    player_t l_localPlayer = FIRST;

    _useCallback( "game$applyInput$begin", _activeMappedKeys, &l_localPlayer );

    if ( l_activeMappedKeysLength ) {
        const int8_t l_directionsValues[][ 2 ] = {
            { -1, -1 }, // 1
            { 0, -1 },  // 2
            { 1, -1 },  // 3
            { -1, 0 },  // 4
            { 0, 0 },   // 5
            { 1, 0 },   // 6
            { -1, 1 },  // 7
            { 0, 1 },   // 8
            { 1, 1 }    // 9
        };

        const char* l_tempKeys[] = { "A",  "B",   "C",   "D",    "E",
                                     "AB", "FN1", "FN2", "START" };
        const size_t l_tempKeysLength =
            ( sizeof( l_tempKeys ) / sizeof( l_tempKeys[ 0 ] ) );

        const button_t l_tempValues[] = {
            ( ( button_t )( ( uint16_t )( A ) | ( uint16_t )( CONFIRM ) ) ),
            ( ( button_t )( ( uint16_t )( B ) | ( uint16_t )( CANCEL ) ) ),
            C,
            D,
            E,
            AB,
            FN1,
            FN2,
            START };

        const size_t l_oldActiveMappedKeysLength =
            ( l_activeMappedKeysLength + 1 );
        size_t l_possibleActiveDirectionsTotal =
            ( ( bool )( 2 ) + ( bool )( 4 ) + ( bool )( 6 ) + ( bool )( 8 ) );
        size_t l_possibleActiveButtonsTotal = l_tempKeysLength;

        l_direction = 5;

        for ( size_t _index = 1; ( ( _index < l_oldActiveMappedKeysLength ) &&
                                   ( ( l_possibleActiveButtonsTotal ) ||
                                     ( l_possibleActiveDirectionsTotal ) ) &&
                                   ( l_activeMappedKeysLength ) );
              _index++ ) {
            const char* l_activeValue = ( *_activeMappedKeys )[ _index ];

            // Direction
            {
                const size_t l_activeValueAsNumber =
                    ( l_activeValue[ 0 ] - '0' );

                if ( ( l_activeValueAsNumber == 2 ) ||
                     ( l_activeValueAsNumber == 4 ) ||
                     ( l_activeValueAsNumber == 6 ) ||
                     ( l_activeValueAsNumber == 8 ) ) {
                    const int8_t* l_directionValue =
                        l_directionsValues[ l_activeValueAsNumber - 1 ];

                    l_direction += ( l_directionValue[ 0 ] +
                                     ( l_directionValue[ 1 ] * 3 ) );

                    l_possibleActiveDirectionsTotal--;
                    l_activeMappedKeysLength--;
                }

#if 0
                if ( l_direction == 5 ) {
                    l_direction = NEUTRAL_DIRECTION;
                }
#endif
            }

            // Buttons
            if ( l_activeMappedKeysLength ) {
                ssize_t l_index;

                if ( ( l_index = findStringInArray(
                           l_tempKeys, l_tempKeysLength, l_activeValue ) ) >=
                     0 ) {
                    const button_t l_value = l_tempValues[ l_index ];

                    l_buttons = ( button_t )( ( uint16_t )( l_buttons ) |
                                              ( uint16_t )( l_value ) );

                    l_possibleActiveButtonsTotal--;
                    l_activeMappedKeysLength--;
                }
            }
        }
    }

    l_returnValue = _useCallback( "game$applyInput$end", &l_buttons,
                                  &l_direction, &l_localPlayer );

    applyInput( l_buttons, l_direction, l_localPlayer );

    return ( l_returnValue );
}
