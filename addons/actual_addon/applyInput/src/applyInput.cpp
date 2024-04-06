#include "applyInput.hpp"

#include <stdio.h>

struct playerInputs_t {
    int64_t _beginOffset1;
    int64_t _beginOffset2;
    int64_t _beginOffset3;
    direction_t firstPlayerDirection;
    int32_t _firstPlayerDirectionOffset1;
    int32_t _firstPlayerDirectionOffset2;
    button_t firstPlayerButtons;
    int32_t firstPlayerButtonsOffset1;
    direction_t secondPlayerDirection;
    int32_t secondPlayerDirectionOffset1;
    button_t secondPlayerButtons;
};

void applyInput( button_t _buttons, direction_t _direction, player_t _player ) {
    char* const l_inputsStructureAddress =
        *( reinterpret_cast< char** >( 0x76E6AC ) );
    // const uintptr_t l_firstPlayerDirectionOffset = 0x18;  // 24
    // const uintptr_t l_firstPlayerButtonsOffset = 0x24;    // 36
    // const uintptr_t l_secondPlayerDirectionOffset = 0x2C; // 44
    // const uintptr_t l_secondPlayerButtonsOffset = 0x38;   // 56

    playerInputs_t* l_playersInputs =
        ( playerInputs_t* )l_inputsStructureAddress;

    switch ( _player ) {
        case FIRST: {
            l_playersInputs->firstPlayerDirection = _direction;
            l_playersInputs->firstPlayerButtons = _buttons;

            break;
        }

        case SECOND: {
            l_playersInputs->secondPlayerDirection = _direction;
            l_playersInputs->secondPlayerButtons = _buttons;

            break;
        }
    }
}
