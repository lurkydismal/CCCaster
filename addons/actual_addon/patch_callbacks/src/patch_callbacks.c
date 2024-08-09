#include "patch_callbacks.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "_useCallback.h"
#include "button_t.h"
#include "direction_t.h"
#include "gameMode_t.h"
#include "misc.h"
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

    do {
        l_length++;
        _number /= 10;
    } while ( _number != 0 );

    return ( l_length );
}

///////////////
/// @brief Converts a long integer to a string.
/// @details Copyright 1988-90 by Robert B. Stout dba MicroFirm. Released to
/// public domain, 1991. Can not convert negative values.
/// @param[in] _number Number to be converted.
/// @return A character pointer to the converted string.
///////////////
char* stoa( size_t _number ) {
///////////////
/// @brief Size of string buffer.
///////////////
#define BUFSIZE ( sizeof( size_t ) * 8 + 1 )

    /// Declare l_characterIndex to register, buffer and pointer of \c l_buffer.
    const size_t l_lengthOfNumber = lengthOfsize( _number );
    char* l_buffer = ( char* )malloc( l_lengthOfNumber + 1 );
    register uint32_t l_characterIndex;
    char *l_tail, *l_head = l_buffer, l_buf[ BUFSIZE ];

    /// Set the last character of string to NULL terminator.
    l_tail = &( l_buf[ BUFSIZE - 1 ] ); // Last character position
    *l_tail-- = '\0';

    /// Convert integer value to string value.
    l_characterIndex = 1;

    do {
        ++l_characterIndex;

        *l_tail-- = ( char )( ( _number % 10 ) + '0' );

        _number /= 10;
    } while ( _number != 0 );

    /// Copy l_tail string to l_head string.
    memcpy( l_head,          // Destination
            ++l_tail,        // Source
            l_characterIndex // Number of bytes
    );

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
            // New frame callback
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
                //  *( ( uint32_t* )( CC_SKIP_FRAMES_ADDR ) ) =
                //  1;

                /*              {
                                    static patch_t forceGotoGameModePatch;

                                    if ( !forceGotoGameModePatch ) {
                                        // Force the game to go to a certain
                   mode
                                        // jmp 0042B4B6
                                        forceGotoGameModePatch.apply( 0x42B475,
                                                                      { 0xEB,
                   0x3F } ); uint16_t l_result = _useCallback( "gameMode$versus"
                   );

                                        // jmp 0042B4D3
                                        // static patch_t forceGotoVersusCPU{
                   0x42B475, {
                                        //  0xEB,
                                        // 0x5C
                                        // }
                                        // };
                                        // uint16_t l_result = _useCallback(
                                        // "gameMode$versusCPU" );

                                        // jmp 0042B499
                                        // static patch_t forceGotoTraining{
                   0x42B475, {
                                        // 0xEB,
                                        // 0x22 } }; uint16_t l_result =
                   _useCallback(
                                        // "gameMode$training" );

                                        // jmp 0042B541
                                        // static patch_t forceGotoReplay{
                                        //     0x42B475, { 0xE9, 0xC7, 0x00,
                   0x00, 0x00 }
                                        //   };
                                        // uint16_t l_result = _useCallback(
                                        // "gameMode$replay"
                                        // );
                                    }
                                }

                                if ( l_framesPassed % 2 ) {
                                    std::set< std::string > l_activeMappedKeys =
                   { "A" };

                                    _useCallback( "keyboard$applyInput", 1,
                   l_activeMappedKeys );
                                }
                                */

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
