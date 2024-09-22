#include "patch_callbacks.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "_useCallback.h"
#include "button_t.h"
#include "direction_t.h"
#include "gameMode_t.h"
#include "misc.h"
#include "patch_t.h"
#include "player_t.h"
#include "stdfunc.h"

uint32_t g_currentMenuIndex = 0;
uint32_t g_menuConfirmState = 0;
bool g_enableEscapeToExit = false;
uint32_t g_roundStartCounter = 0;
uint8_t g_SFXMute[ CC_SFX_ARRAY_LENGTH ] = { 0 };
uint8_t g_SFXFilter[ CC_SFX_ARRAY_LENGTH ] = { 0 };
uint32_t* g_autoReplaySaveState;

static char* gameModeToText( gameMode_t _gameMode ) {
    char* l_returnValue;

    switch ( _gameMode ) {
        case STARTUP: {
            l_returnValue = "startup";

            break;
        }

        case OPENING: {
            l_returnValue = "opening";

            break;
        }

        case TITLE: {
            l_returnValue = "title";

            break;
        }

        case MAIN_MENU: {
            l_returnValue = "main_menu";

            break;
        }

        case CHARACTER_SELECT: {
            l_returnValue = "character_select";

            break;
        }

        case LOADING: {
            l_returnValue = "loading";

            break;
        }

        case IN_MATCH: {
            l_returnValue = "in_match";

            break;
        }

        case RETRY: {
            l_returnValue = "retry";

            break;
        }

        case LOADING_DEMO: {
            l_returnValue = "loading_demo";

            break;
        }

        case REPLAY: {
            l_returnValue = "replay";

            break;
        }

        case HIGH_SCORES: {
            l_returnValue = "high_scores";

            break;
        }

        default: {
            l_returnValue = "UNKNOWN - Report this!";
        }
    }

    return ( l_returnValue );
}

void extraTexturesCallBack( void ) {
    uint16_t l_result = _useCallback( "extraTextures" );
}

void characterSelectColorsCallback( void ) {
    uint16_t l_result = _useCallback( "characterSelectColors" );
}

void loadingColorsCallback( void ) {
    uint16_t l_result = _useCallback( "loadingColors" );
}

void extraDrawCallback( void ) {
    uint16_t l_result = _useCallback( "extraDraw" );
}

void gameMainLoopCallback( void ) {
    // Frame step timer, always counting up
#define CC_WORLD_TIMER_ADDR ( 0x55D1D4 )

    static uint32_t l_framesPassed = 0;
    bool l_isNewFrame = false;

    if ( l_framesPassed != *( ( uint32_t* )( CC_WORLD_TIMER_ADDR ) ) ) {
        l_framesPassed = *( ( uint32_t* )( CC_WORLD_TIMER_ADDR ) );

        l_isNewFrame = true;
    }

#undef CC_WORLD_TIMER_ADDR

    if ( l_isNewFrame ) {
        {
            uint16_t l_result = _useCallback( "mainLoop$newFrame" );
        }

        static uint32_t l_roundStartCounter = 0;

        if ( l_roundStartCounter != g_roundStartCounter ) {
            l_roundStartCounter = g_roundStartCounter;

            uint16_t l_result = _useCallback(
                "match$newRound", &g_roundStartCounter, &l_roundStartCounter );
        }

        static gameMode_t l_currentGameMode = STARTUP;

        if ( l_currentGameMode != *( ( gameMode_t* )( CC_GAME_MODE_ADDR ) ) ) {
            l_currentGameMode = *( ( gameMode_t* )( CC_GAME_MODE_ADDR ) );

            char* l_currentGameModeAsString = stoa( l_currentGameMode );
            char* l_currentGameModeAsText = gameModeToText( l_currentGameMode );

            _useCallback( "log$transaction$query", "gameMode$changed : " );
            _useCallback( "log$transaction$query", l_currentGameModeAsString );
            _useCallback( "log$transaction$query", " " );
            _useCallback( "log$transaction$query", l_currentGameModeAsText );
            _useCallback( "log$transaction$query", "\n" );

            free( l_currentGameModeAsString );

            uint16_t l_result =
                _useCallback( "gameMode$changed", &l_currentGameMode,
                              l_currentGameModeAsText );
        }

        switch ( l_currentGameMode ) {
            case STARTUP: {
                uint16_t l_result = _useCallback( "gameMode$startup" );

                break;
            }

            case OPENING: {
                *( ( uint32_t* )( CC_SKIP_FRAMES_ADDR ) ) = 1;

                uint16_t l_result = _useCallback( "gameMode$opening" );

                break;
            }

            case TITLE: {
                *( ( uint32_t* )( CC_SKIP_FRAMES_ADDR ) ) = 1;

                char** l_input = ( char** )createArray( sizeof( char* ) );
                const char l_button[] = "A";

                insertIntoArray( ( void*** )( &l_input ),
                                 ( void* )( l_button ) );

                _useCallback( "game$applyInput", &l_input );

                free( l_input );

                uint16_t l_result = _useCallback( "gameMode$title" );

                break;
            }

            case MAIN_MENU: {
#if 0
                *( ( uint32_t* )( CC_SKIP_FRAMES_ADDR ) ) = 1;

                {
                    static patch_t forceGotoGameModePatch;

                    if ( !( forceGotoGameModePatch.bytesBackupLength ) ) {
                        {
                            // Force the game to go to a certain mode
                            // jmp 0042B4B6
                            MAKE_PATCH_WITH_RETURN( forceGotoGameModePatch,
                                                    0x42B475, { 0xEB, 0x3F } );

                            uint16_t l_result =
                                _useCallback( "gameMode$versus" );
                        }

                        {
                            // jmp 0042B4D3
                            MAKE_PATCH_WITH_RETURN( forceGotoGameModePatch,
                                                    0x42B475, { 0xEB, 0x5C } );

                            uint16_t l_result =
                                _useCallback( "gameMode$versusCPU" );
                        }

                        {
                            // jmp 0042B499
                            MAKE_PATCH_WITH_RETURN( forceGotoGameModePatch,
                                                    0x42B475, { 0xEB, 0x22 } );

                            uint16_t l_result =
                                _useCallback( "gameMode$training" );
                        }

                        {
                            // jmp 0042B541
                            MAKE_PATCH_WITH_RETURN(
                                forceGotoGameModePatch, 0x42B475,
                                { 0xE9, 0xC7, 0x00, 0x00, 0x00 } );

                            uint16_t l_result =
                                _useCallback( "gameMode$replay" );
                        }
                    }
                }

                if ( l_framesPassed % 2 ) {
                    char** l_input = ( char** )createArray( sizeof( char* ) );

                    insertIntoArray( ( void*** )( &l_input ), ( void* )( "A" ) );

                    _useCallback( "game$applyInput", &l_input );

                    free( l_input );
                }
#endif

                uint16_t l_result = _useCallback( "gameMode$main" );

                break;
            }

            case CHARACTER_SELECT: {
                uint16_t l_result = _useCallback( "gameMode$characterSelect" );

                break;
            }

            case LOADING: {
                uint16_t l_result = _useCallback( "gameMode$loading" );

                break;
            }

            case IN_MATCH: {
                uint16_t l_result = _useCallback( "gameMode$inMatch" );

                break;
            }
        }
    }

    uint16_t l_result = _useCallback( "mainLoop$end" );
}
