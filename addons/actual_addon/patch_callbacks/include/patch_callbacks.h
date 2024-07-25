#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "misc.h"

extern uint32_t g_currentMenuIndex;
extern uint32_t g_menuConfirmState;
extern bool g_enableEscapeToExit;
extern uint32_t g_roundStartCounter;
extern uint8_t g_SFXMute[ CC_SFX_ARRAY_LENGTH ];
extern uint8_t g_SFXFilter[ CC_SFX_ARRAY_LENGTH ];
extern uint32_t* g_autoReplaySaveState;

void extraTexturesCallBack( void );
void characterSelectColorsCallback( void );
void loadingColorsCallback( void );
void extraDrawCallback( void );
void gameMainLoopCallback( void );
