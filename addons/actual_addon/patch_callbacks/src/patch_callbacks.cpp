#include "patch_callbacks.hpp"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <set>
#include <string>

#include "_useCallback.hpp"
#include "a.h"
#include "button_input.h"
#include "direction_input.h"
#include "game_mode.h"
#include "patch_t.hpp"
#include "player_t.h"

#define HEAP_MEMORY_SIZE 150

uint32_t g_currentMenuIndex = 0;
uint32_t g_menuConfirmState = 0;
bool g_enableEscapeToExit = false;
uint32_t g_roundStartCounter = 0;
uint8_t g_SFXMute[ CC_SFX_ARRAY_LENGTH ] = { 0 };
uint8_t g_SFXFilter[ CC_SFX_ARRAY_LENGTH ] = { 0 };
uint32_t* g_autoReplaySaveState;

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

    static uint32_t l_framesPassed =
        *( reinterpret_cast< uint32_t* >( CC_WORLD_TIMER_ADDR ) );
    bool l_isNewFrame = false;

    if ( l_framesPassed !=
         *( reinterpret_cast< uint32_t* >( CC_WORLD_TIMER_ADDR ) ) ) {
        l_framesPassed =
            *( reinterpret_cast< uint32_t* >( CC_WORLD_TIMER_ADDR ) );

        l_isNewFrame = true;
    }

#undef CC_WORLD_TIMER_ADDR

    if ( l_isNewFrame ) {
        {
            // New frame callback
            uint16_t l_result = _useCallback( "mainLoop$newFrame" );
        }

        static uint32_t l_roundStartCounter = g_roundStartCounter;
        bool l_isNewRound = false;

        if ( l_roundStartCounter != g_roundStartCounter ) {
            l_roundStartCounter = g_roundStartCounter;

            l_isNewRound = true;
        }

        static gameMode_t l_currentGameMode = STARTUP;

        if ( l_currentGameMode !=
             *( reinterpret_cast< gameMode_t* >( CC_GAME_MODE_ADDR ) ) ) {
            l_currentGameMode =
                *( reinterpret_cast< gameMode_t* >( CC_GAME_MODE_ADDR ) );

            printf( "%d\n", l_currentGameMode );

            uint16_t l_result =
                _useCallback( "gameMode$changed", 1, l_currentGameMode );
        }

        switch ( l_currentGameMode ) {
            case STARTUP: {
                uint16_t l_result = _useCallback( "gameMode$startup" );

                break;
            }

            case OPENING: {
                *( reinterpret_cast< uint32_t* >( CC_SKIP_FRAMES_ADDR ) ) = 1;

                uint16_t l_result = _useCallback( "gameMode$opening" );

                return;
            }

            case TITLE: {
                *( reinterpret_cast< uint32_t* >( CC_SKIP_FRAMES_ADDR ) ) = 1;

                std::set< std::string > l_activeMappedKeys = { "A" };

                _useCallback( "keyboard$applyInput", 1, l_activeMappedKeys );

                uint16_t l_result = _useCallback( "gameMode$title" );
            }

            case MAIN: {
                //  *( reinterpret_cast< uint32_t* >( CC_SKIP_FRAMES_ADDR ) ) =
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
                   { "A" }; std::set< std::string > l_activeKeys = {};

                                    _useCallback( "keyboard$applyInput", 2,
                   l_activeMappedKeys, l_activeKeys );
                                }

                                uint16_t l_result = _useCallback(
                   "gameMode$main" );

                                if ( l_framesPassed % 2 ) {
                                    return;
                                }
                */
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
                    printf( "l_isNewRound\n" );

                    uint16_t l_result = _useCallback( "match$newRound" );
                }

                break;
            }
        }
    }

    uint16_t l_result = _useCallback( "mainLoop$end" );
}
