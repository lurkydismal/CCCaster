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

uint32_t g_currentMenuIndex = 0;
uint32_t g_menuConfirmState = 0;
bool g_enableEscapeToExit = false;
uint32_t g_roundStartCounter = 0;
uint8_t g_SFXMute[ CC_SFX_ARRAY_LENGTH ] = { 0 };
uint8_t g_SFXFilter[ CC_SFX_ARRAY_LENGTH ] = { 0 };
uint32_t* g_autoReplaySaveState;

size_t lengthOfsize( size_t _number ) {
    size_t l_length = 0;

#pragma GCC ivdep
    do {
        l_length++;
        _number /= 10;
    } while ( _number != 0 );

    return ( l_length );
}

char* stoa( size_t _number ) {
#define BUFSIZE ( sizeof( size_t ) * 8 + 1 )

    const size_t l_lengthOfNumber = lengthOfsize( _number );
    char* l_buffer = ( char* )malloc( l_lengthOfNumber + 1 );
    register uint32_t l_characterIndex;
    char *l_tail, *l_head = l_buffer, l_buf[ BUFSIZE ];

    l_tail = &( l_buf[ BUFSIZE - 1 ] );
    *l_tail-- = '\0';

    l_characterIndex = 1;

#pragma GCC ivdep
    for ( l_characterIndex = 1; _number != 0; _number /= 10 ) {
        ++l_characterIndex;

        *l_tail-- = ( char )( ( _number % 10 ) + '0' );
    }

    memcpy( l_head, ++l_tail, l_characterIndex );

    return ( l_buffer );

#undef BUFSIZE
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
        bool l_isNewRound = false;

        if ( l_roundStartCounter != g_roundStartCounter ) {
            l_roundStartCounter = g_roundStartCounter;

            l_isNewRound = true;
        }

        static gameMode_t l_currentGameMode = STARTUP;

        if ( l_currentGameMode != *( ( gameMode_t* )( CC_GAME_MODE_ADDR ) ) ) {
            l_currentGameMode = *( ( gameMode_t* )( CC_GAME_MODE_ADDR ) );

            {
                char* l_currentGameModeAsText = stoa( l_currentGameMode );
                const size_t l_currentGameModeAsTextLength =
                    strlen( l_currentGameModeAsText );
                l_currentGameModeAsText = ( char* )realloc(
                    l_currentGameModeAsText,
                    ( l_currentGameModeAsTextLength + 1 + 1 ) );
                l_currentGameModeAsText[ l_currentGameModeAsTextLength ] = '\n';
                l_currentGameModeAsText[ l_currentGameModeAsTextLength + 1 ] =
                    '\0';

                _useCallback( "log$transaction$query",
                              l_currentGameModeAsText );
                _useCallback( "log$transaction$commit" );

                free( l_currentGameModeAsText );
            }

            uint16_t l_result =
                _useCallback( "gameMode$changed", &l_currentGameMode );
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

                const button_t l_button = A;
                const direction_t l_direction = NEUTRAL_DIRECTION;
                const player_t l_player = FIRST;
                _useCallback( "game$applyInput", &l_button, &l_direction,
                              &l_player );

                uint16_t l_result = _useCallback( "gameMode$title" );

                break;
            }

            case MAIN: {
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
                    const button_t l_button = A;
                    const direction_t l_direction = NEUTRAL_DIRECTION;
                    const player_t l_player = FIRST;
                    _useCallback( "game$applyInput", &l_button, &l_direction,
                            &l_player );
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

                if ( l_isNewRound ) {
                    uint16_t l_result = _useCallback( "match$newRound" );
                }

                break;
            }
        }
    }

    uint16_t l_result = _useCallback( "mainLoop$end" );
}
