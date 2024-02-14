#include "patch_callbacks.hpp"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <string>

#include "_useCallback.hpp"
#include "a.h"
#include "addon_callbacks.hpp"
#include "button_input.h"
#include "direction_input.h"
#include "gameMode.h"
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

static void applyInput( button_t _buttons,
                        uint32_t _direction = 0,
                        uint32_t _player = 1 ) {
    char* const l_inputsStructureAddress =
        *( reinterpret_cast< char** >( 0x76E6AC ) );
    const uintptr_t l_firstPlayerDirectionOffset = 0x18;  // 24
    const uintptr_t l_firstPlayerButtonsOffset = 0x24;    // 36
    const uintptr_t l_secondPlayerDirectionOffset = 0x2C; // 44
    const uintptr_t l_secondPlayerButtonsOffset = 0x38;   // 56

    switch ( _player ) {
        case 1: {
            *( reinterpret_cast< uint16_t* >( l_inputsStructureAddress +
                                              l_firstPlayerDirectionOffset ) ) =
                _direction;
            *( reinterpret_cast< uint16_t* >( l_inputsStructureAddress +
                                              l_firstPlayerButtonsOffset ) ) =
                _buttons;

            break;
        }

        case 2: {
            *( reinterpret_cast< uint16_t* >(
                l_inputsStructureAddress + l_secondPlayerDirectionOffset ) ) =
                _direction;
            *( reinterpret_cast< uint16_t* >( l_inputsStructureAddress +
                                              l_secondPlayerButtonsOffset ) ) =
                _buttons;

            break;
        }
    }
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
        static uint32_t l_roundStartCounter = g_roundStartCounter;
        bool l_isNewRound = false;

        if ( l_roundStartCounter != g_roundStartCounter ) {
            l_roundStartCounter = g_roundStartCounter;

            l_isNewRound = true;
        }

        static gameMode_t l_currentGameMode = STARTUP;
        static bool l_disableMenu = false;

        if ( l_currentGameMode !=
             *( reinterpret_cast< gameMode_t* >( CC_GAME_MODE_ADDR ) ) ) {
            l_currentGameMode =
                *( reinterpret_cast< gameMode_t* >( CC_GAME_MODE_ADDR ) );

            printf( "%d\n", l_currentGameMode );

            uint16_t l_result =
                _useCallback( "gameMode$changed", 1, l_currentGameMode );
        }

        switch ( l_currentGameMode ) {
            case OPENING: {
                *( reinterpret_cast< uint32_t* >( CC_SKIP_FRAMES_ADDR ) ) = 1;

                uint16_t l_result = _useCallback( "gameMode$opening" );

                return;
            }

            case TITLE: {
                *( reinterpret_cast< uint32_t* >( CC_SKIP_FRAMES_ADDR ) ) = 1;

                applyInput( static_cast< button_t >( A | CONFIRM ) );

                uint16_t l_result = _useCallback( "gameMode$title" );

                return;
            }

            case MAIN: {
                *( reinterpret_cast< uint32_t* >( CC_SKIP_FRAMES_ADDR ) ) = 1;

                // Force the game to go to a certain mode
                // jmp 0042B4B6
                // static patch_t forceGotoVersus{ 0x42B475, { 0xEB, 0x3F } };
                // l_disableMenu = true;

                // jmp 0042B4D3
                // static patch_t forceGotoVersusCPU{ 0x42B475, { 0xEB, 0x5C }
                // };

                // jmp 0042B499
                static patch_t forceGotoTraining{ 0x42B475, { 0xEB, 0x22 } };

                // jmp 0042B541
                // static patch_t forceGotoReplay{
                //     0x42B475, { 0xE9, 0xC7, 0x00, 0x00, 0x00 } };

                if ( l_framesPassed % 2 ) {
                    applyInput( static_cast< button_t >( A | CONFIRM ) );
                }

                uint16_t l_result = _useCallback( "gameMode$main" );

                if ( l_framesPassed % 2 ) {
                    return;
                }

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

#if defined( CLIENT )

        player_t l_localPlayer = SECOND;

#else // CLIENT

        player_t l_localPlayer = FIRST;

#endif // CLIENT

        direction_t l_direction = NEUTRAL_DIRECTION;
        button_t l_buttons = NEUTRAL_BUTTON;

        if ( ( GetActiveWindow() == g_hFocusWindow ) &&
             ( g_hFocusWindow != NULL ) ) {
            uint16_t l_result =
                _useCallback( "mainLoop$getLocalInput", 3, l_localPlayer,
                              &l_direction, &l_buttons );

#if defined( CLIENT ) || defined( SERVER )

            printf( "1 %d %d %d\n", l_buttons, l_direction, l_localPlayer );

#endif // CLIENT || SERVER
        }

        if ( l_disableMenu ) {
            l_buttons = static_cast< button_t >(
                static_cast< uint16_t >( l_buttons ) &
                ~( static_cast< uint16_t >( START ) ) );
        }

        applyInput( l_buttons, l_direction, l_localPlayer );

#if defined( CLIENT ) || defined( SERVER )

        char l_allocatedMemory[ HEAP_MEMORY_SIZE ];

        char* l_secondPlayerInput = &( l_allocatedMemory[ 0 ] );

        *( reinterpret_cast< uint16_t* >( l_secondPlayerInput ) ) =
            COMBINE_INPUT( l_buttons, l_direction );

        printf( "%d\n", COMBINE_INPUT( l_buttons, l_direction ) );

        send( g_connectSocket, l_secondPlayerInput, sizeof( uint16_t ), 0 );

        recv( g_connectSocket, l_secondPlayerInput, sizeof( uint16_t ), 0 );

#if defined( CLIENT )

        player_t l_remotePlayer = FIRST;

#elif defined( SERVER )

        player_t l_remotePlayer = SECOND;

#endif // CLIENT

        printf( "3 %d %d %d\n",
                INLINE_INPUT(
                    *( reinterpret_cast< uint16_t* >( l_secondPlayerInput ) ) ),
                l_remotePlayer );

        applyInput( INLINE_INPUT( *( reinterpret_cast< uint16_t* >(
                        l_secondPlayerInput ) ) ),
                    l_remotePlayer );

#endif // CLIENT || SERVER
    }
}
