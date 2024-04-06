#pragma once

#include "button_input.h"
#include "direction_input.h"
#include "player_t.h"

void applyInput( button_t _buttons = NEUTRAL_BUTTON,
                 direction_t _direction = NEUTRAL_DIRECTION,
                 player_t _player = FIRST );
