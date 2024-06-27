#include "addon_callbacks.hpp"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <cstdint>
#include <set>

#include "_useCallback.hpp"
#include "button_input.h"
#include "controls_parse.hpp"
#include "d3d9.h"
#include "d3dx9.h"
#include "direction_input.h"
#include "native.hpp"
#include "player_t.h"

#pragma comment( lib, "user32.lib" )

namespace {

uint32_t g_activeFlagsKeyboard = 0;
bool g_disableMenu = false;
const std::map< uint32_t, std::string > g_keyboardLayout = {
    { VK_ESCAPE, "Esc" },
    { VK_F1, "F1" },
    { VK_F2, "F2" },
    { VK_F3, "F3" },
    { VK_F4, "F4" },
    { VK_F5, "F5" },
    { VK_F6, "F6" },
    { VK_F7, "F7" },
    { VK_F8, "F8" },
    { VK_F9, "F9" },
    { VK_F10, "F10" },
    { VK_F11, "F11" },
    { VK_F12, "F12" },
    { VK_PAUSE, "Pause" },
    { VK_OEM_3, "~" },
    { 0x31, "1" },
    { 0x32, "2" },
    { 0x33, "3" },
    { 0x34, "4" },
    { 0x35, "5" },
    { 0x36, "6" },
    { 0x37, "7" },
    { 0x38, "8" },
    { 0x39, "9" },
    { 0x30, "0" },
    { VK_OEM_MINUS, "-" },
    { VK_OEM_PLUS, "+" },
    { VK_BACK, "Backspace" },
    { VK_INSERT, "Insert" },
    { VK_HOME, "Home" },
    { VK_PRIOR, "PgUp" },
    { VK_TAB, "Tab" },
    { 0x51, "Q" },
    { 0x57, "W" },
    { 0x45, "E" },
    { 0x52, "R" },
    { 0x54, "T" },
    { 0x59, "Y" },
    { 0x55, "U" },
    { 0x49, "I" },
    { 0x4F, "O" },
    { 0x50, "P" },
    { VK_OEM_4, "[" },
    { VK_OEM_6, "]" },
    { VK_OEM_5, "|" },
    { VK_DELETE, "Del" },
    { VK_END, "End" },
    { VK_NEXT, "PgDn" },
    { VK_CAPITAL, "Caps Lock" },
    { 0x41, "A" },
    { 0x53, "S" },
    { 0x44, "D" },
    { 0x46, "F" },
    { 0x47, "G" },
    { 0x48, "H" },
    { 0x4A, "J" },
    { 0x4B, "K" },
    { 0x4C, "L" },
    { VK_OEM_1, ";" },
    { VK_OEM_7, "\'" },
    { VK_RETURN, "Enter" },
    { VK_LSHIFT, "L.Shift" },
    { 0x5A, "Z" },
    { 0x58, "X" },
    { 0x43, "C" },
    { 0x56, "V" },
    { 0x42, "B" },
    { 0x4E, "N" },
    { 0x4D, "M" },
    { VK_OEM_COMMA, "," },
    { VK_OEM_PERIOD, "." },
    { VK_OEM_2, "/" },
    { VK_RSHIFT, "R.Shift" },
    { VK_UP, "Up" },
    { VK_LCONTROL, "L.Ctrl" },
    { VK_LMENU, "L.Alt" },
    { VK_SPACE, "Space" },
    { VK_RMENU, "R.Alt" },
    { VK_RCONTROL, "R.Ctrl" },
    { VK_LEFT, "Left" },
    { VK_DOWN, "Down" },
    { VK_RIGHT, "Right" } };

HWND g_hFocusWindow = NULL;
uint32_t g_framesPassed = 0;
uint16_t g_menuCursorIndex = 0;

} // namespace

useCallbackFunction_t g_useCallback = NULL;

void applyInput( button_t _buttons, direction_t _direction, player_t _player ) {
    char* const l_inputsStructureAddress =
        *( reinterpret_cast< char** >( 0x76E6AC ) );
    // const uintptr_t l_firstPlayerDirectionOffset = 0x18;  // 24
    // const uintptr_t l_firstPlayerButtonsOffset = 0x24;    // 36
    // const uintptr_t l_secondPlayerDirectionOffset = 0x2C; // 44
    // const uintptr_t l_secondPlayerButtonsOffset = 0x38;   // 56

    *( uint16_t* )( l_inputsStructureAddress + 16 +
                    ( 20 * ( ( uint8_t )_player ) ) ) =
        ( uint16_t )( _buttons );
    *( uint16_t* )( l_inputsStructureAddress + 4 +
                    ( 20 * ( ( uint8_t )_player ) ) ) =
        ( uint16_t )( _direction );
}

extern "C" uint16_t __declspec( dllexport )
    mainLoop$newFrame( void** _callbackArguments ) {
    uint16_t l_returnValue = 0;

    if ( ( GetActiveWindow() != g_hFocusWindow ) ||
         ( g_hFocusWindow == NULL ) ) {
        return ( l_returnValue );
    }

    if ( g_framesPassed < ( UINT32_MAX - 1 ) ) {
        g_framesPassed++;
    }

    std::set< std::string > l_activeMappedKeys;
    std::set< uint8_t > l_activeKeys;

#define KEYS_TOTAL ( 0xFE + 1 )

    for ( uint8_t _index = 0x0D; _index < KEYS_TOTAL; _index++ ) {
        if ( !( _index == VK_MENU ) && !( _index == VK_SHIFT ) &&
             !( _index == VK_CONTROL ) ) {
            if ( GetKeyState( _index ) & 0x8000 ) {
                const std::string l_indexAsString = std::to_string( _index );

                if ( g_jsonControlsKeyboard.contains( l_indexAsString ) ) {
                    l_activeMappedKeys.insert(
                        g_jsonControlsKeyboard.at( l_indexAsString ) );

                } else {
                    l_activeKeys.insert( _index );
                }
            }
        }
    }

#undef KEY_NUMBER

    l_returnValue = _useCallback( "keyboard$getInput$end", 2,
                                  &l_activeMappedKeys, &l_activeKeys );
    l_returnValue =
        _useCallback( "keyboard$applyInput", 1, &l_activeMappedKeys );

    return ( l_returnValue );
}

extern "C" uint16_t __declspec( dllexport )
    keyboard$applyInput( void** _callbackArguments ) {
    uint16_t l_returnValue = 0;
    std::set< std::string >* _activeMappedKeys =
        ( std::set< std::string >* )_callbackArguments[ 0 ];
    direction_t l_direction = NEUTRAL_DIRECTION;
    button_t l_buttons = NEUTRAL_BUTTON;

    l_returnValue =
        _useCallback( "keyboard$applyInput$begin", 1, *_activeMappedKeys );

    if ( _activeMappedKeys->find( "8" ) != _activeMappedKeys->end() ) {
        l_direction = UP;

    } else if ( _activeMappedKeys->find( "2" ) != _activeMappedKeys->end() ) {
        l_direction = DOWN;
    }

    if ( _activeMappedKeys->find( "4" ) != _activeMappedKeys->end() ) {
        l_direction = ( l_direction )
                          ? static_cast< direction_t >(
                                ( static_cast< uint8_t >( l_direction ) ) - 1 )
                          : LEFT;

    } else if ( _activeMappedKeys->find( "6" ) != _activeMappedKeys->end() ) {
        l_direction = ( l_direction )
                          ? static_cast< direction_t >(
                                ( static_cast< uint8_t >( l_direction ) ) + 1 )
                          : RIGHT;
    }

    const std::set< std::pair< std::string, button_t > > l_temp = {
        { "A",
          ( static_cast< button_t >( static_cast< uint16_t >( A ) |
                                     static_cast< uint16_t >( CONFIRM ) ) ) },
        { "B",
          ( static_cast< button_t >( static_cast< uint16_t >( B ) |
                                     static_cast< uint16_t >( CANCEL ) ) ) },
        { "C", C },
        { "D", D },
        { "E", E },
        { "AB", AB },
        { "FN1", FN1 },
        { "FN2", FN2 },
        { "START", START } };

    for ( std::pair< std::string, button_t > const& _button : l_temp ) {
        if ( _activeMappedKeys->find( _button.first ) !=
             _activeMappedKeys->end() ) {
            l_buttons = static_cast< button_t >(
                static_cast< uint16_t >( l_buttons ) |
                static_cast< uint16_t >( _button.second ) );
        }
    }

    {
        // 6 - 0110
        // 4 - 0100
        // 2 - 0010
        if ( ( l_direction == 0 ) || ( l_direction & 0b1001 ) ) {
            l_buttons =
                static_cast< button_t >( static_cast< uint16_t >( l_buttons ) &
                                         ~static_cast< uint16_t >( AB ) );
        }
    }

    {
        if ( g_disableMenu ) {
            l_buttons = static_cast< button_t >(
                static_cast< uint16_t >( l_buttons ) &
                ~( static_cast< uint16_t >( START ) ) );
        }
    }

    {
        if ( g_framesPassed >= 30 ) {
            if ( _activeMappedKeys->find( "ToggleOverlay_KeyConfig_Native" ) !=
                 _activeMappedKeys->end() ) {
                g_activeFlagsKeyboard ^= SHOW_OVERLAY_NATIVE;

                g_framesPassed = 0;
            }
        }
    }

    {
        player_t l_localPlayer = FIRST;

        if ( !( g_activeFlagsKeyboard & SHOW_OVERLAY_NATIVE ) ) {
            applyInput( l_buttons, l_direction, l_localPlayer );
        }
    }

    l_returnValue =
        _useCallback( "keyboard$applyInput$end", 1, _activeMappedKeys );

    return ( l_returnValue );
}

extern "C" uint16_t __declspec( dllexport )
    gameMode$versus( void** _callbackArguments ) {
    g_disableMenu = true;

    return ( 0 );
}

extern "C" uint16_t __declspec( dllexport )
    IDirect3D9Ex$CreateDevice( void** _callbackArguments ) {
    HWND* _hFocusWindow = ( HWND* )_callbackArguments[ 2 ];
    D3DPRESENT_PARAMETERS** _pPresentationParameters =
        ( D3DPRESENT_PARAMETERS** )_callbackArguments[ 4 ];

    g_hFocusWindow = ( *_hFocusWindow )
                         ? ( *_hFocusWindow )
                         : ( ( *_pPresentationParameters )->hDeviceWindow );

    HMODULE l_statesDll = GetModuleHandleA( "states.dll" );

    if ( !l_statesDll ) {
        exit( 1 );
    }

    g_useCallback = reinterpret_cast< useCallbackFunction_t >(
        GetProcAddress( l_statesDll, "useCallback" ) );

    if ( !g_useCallback ) {
        exit( 1 );
    }

    return ( 0 );
}

extern "C" uint16_t __declspec( dllexport )
    keyboard$getInput$end( void** _callbackArguments ) {
    if ( g_framesPassed > 7 ) {
        if ( g_activeFlagsKeyboard & OVERLAY_IS_MAPPING_KEY ) {
            std::set< std::string >* _activeMappedKeys =
                ( std::set< std::string >* )_callbackArguments[ 0 ];
            std::set< uint8_t >* _activeKeys =
                ( std::set< uint8_t >* )_callbackArguments[ 1 ];

            if ( _activeMappedKeys->find( "B" ) != _activeMappedKeys->end() ) {
                g_activeFlagsKeyboard &= ~OVERLAY_IS_MAPPING_KEY;

                g_framesPassed = 0;
            }

            if ( !_activeKeys->empty() ) {
                if ( g_keyboardLayout.find( *( _activeKeys->begin() ) ) !=
                     g_keyboardLayout.end() ) {
                    uint16_t l_index = 0;

                    for ( const auto& _item : g_jsonControlsKeyboard.items() ) {
                        if ( l_index == g_menuCursorIndex ) {
                            g_jsonControlsKeyboard[ std::to_string(
                                *( _activeKeys->begin() ) ) ] =
                                _item.value().template get< std::string >();

                            g_jsonControlsKeyboard.erase( _item.key() );

                            const std::string l_controlsConfigFileName =
                                ( std::string(
                                      CONTROLS_PREFERENCES_FILE_NAME ) +
                                  std::string( "." ) +
                                  std::string(
                                      CONTROLS_PREFERENCES_FILE_EXTENSION ) );

                            json l_jsonControls = {
                                { "keyboard", g_jsonControlsKeyboard },
                            };

                            std::string buffer =
                                l_jsonControls.dump( 4 ) + std::string( "\n" );

                            writeFile( l_controlsConfigFileName.c_str(),
                                       buffer.c_str() );

                            break;
                        }

                        l_index++;
                    }
                }

                g_menuCursorIndex = ( g_jsonControlsKeyboard.size() - 1 );

                g_activeFlagsKeyboard &= ~OVERLAY_IS_MAPPING_KEY;

                g_framesPassed = 0;
            }

        } else if ( g_activeFlagsKeyboard & SHOW_OVERLAY_NATIVE ) {
            std::set< std::string >* _activeMappedKeys =
                ( std::set< std::string >* )_callbackArguments[ 0 ];

            if ( _activeMappedKeys->find( "8" ) != _activeMappedKeys->end() ) {
                g_menuCursorIndex--;

                if ( g_menuCursorIndex >
                     ( g_jsonControlsKeyboard.size() - 1 ) ) {
                    g_menuCursorIndex = ( g_jsonControlsKeyboard.size() - 1 );
                }

                g_framesPassed = 0;
            } else if ( _activeMappedKeys->find( "2" ) !=
                        _activeMappedKeys->end() ) {
                g_menuCursorIndex++;

                if ( g_menuCursorIndex >
                     ( g_jsonControlsKeyboard.size() - 1 ) ) {
                    g_menuCursorIndex = 0;
                }

                g_framesPassed = 0;
            }

            if ( _activeMappedKeys->find( "A" ) != _activeMappedKeys->end() ) {
                g_activeFlagsKeyboard |= OVERLAY_IS_MAPPING_KEY;

                g_framesPassed = 0;

            } else if ( _activeMappedKeys->find( "B" ) !=
                        _activeMappedKeys->end() ) {
                g_activeFlagsKeyboard &= ~OVERLAY_IS_MAPPING_KEY;

                g_framesPassed = 0;
            }
        }
    }

    return ( 0 );
}

extern "C" uint16_t __declspec( dllexport )
    extraDrawCallback( void** _callbackArguments ) {
    static bool l_animationNeeded = true;

    if ( g_activeFlagsKeyboard & SHOW_OVERLAY_NATIVE ) {
        const uint8_t l_maxFramesPerSecond = 60;
        const float l_transitionTimeInSeconds = 0.2;
        static int32_t l_overlayY = 0;
        RECT l_tempRectangle;
        static uint32_t l_windowHeight = 0;

        if ( GetWindowRect( g_hFocusWindow, &l_tempRectangle ) ) {
            uint32_t l_tempHeight =
                ( l_tempRectangle.bottom - l_tempRectangle.top );

            if ( ( l_overlayY < 0 ) && ( l_windowHeight ) &&
                 ( l_windowHeight != l_tempHeight ) ) {
                l_overlayY = ( ( l_tempHeight + l_overlayY ) * -1 );
            }

            l_windowHeight = l_tempHeight;

            if ( l_animationNeeded ) {
                l_overlayY = ( l_windowHeight * -1 );

                l_animationNeeded = false;
            }
        }

        struct rectangle {
            uint16_t layer;
            colorsForRectangle_t colorsForRectangle;
            coordinates_t coordinates;
            struct size size;
        };

        struct text {
            uint16_t layer;
            uint8_t alpha;
            uint16_t shade;
            uint16_t shade2;
            coordinates_t coordinates;
            struct size size;
            const std::string content;
        };

        std::vector< struct rectangle > l_rectangles;
        std::vector< struct text > l_texts;

        // Logic
        {
            // Background
            {
                const uint8_t l_alpha = 65;
                const uint8_t l_red = 0xDA;
                const uint8_t l_green = 0xDA;
                const uint8_t l_blue = 0xDA;
                uint32_t l_color = 0;
                struct rectangle l_background;

                _useCallback( "native$getColorForRectangle", 5, &l_alpha,
                              &l_red, &l_green, &l_blue, &l_color );

                l_background.colorsForRectangle = { l_color, l_color, l_color,
                                                    l_color };

                l_background.layer = ( 0 );

                l_background.size = { *( uint32_t* )SCREEN_WIDTH,
                                      l_windowHeight };

                if ( l_overlayY < 0 ) {
                    l_overlayY += ( l_background.size.height /
                                    ( l_transitionTimeInSeconds *
                                      l_maxFramesPerSecond ) );
                }

                if ( l_overlayY > 0 ) {
                    l_overlayY = ( -1 * ( l_background.size.height /
                                          ( l_transitionTimeInSeconds *
                                            l_maxFramesPerSecond ) ) );
                }

                l_background.coordinates = { 0, l_overlayY };

                l_rectangles.push_back( l_background );
            }

            // UI
            {
                struct rectangle l_textBackground;

                // Text background
                {
                    const uint8_t l_alpha = 0xFF;
                    const uint8_t l_red = 0;
                    const uint8_t l_green = 0;
                    const uint8_t l_blue = 0;
                    uint32_t l_color = 0;

                    _useCallback( "native$getColorForRectangle", 5, &l_alpha,
                                  &l_red, &l_green, &l_blue, &l_color );

                    l_textBackground.colorsForRectangle = { l_color, l_color,
                                                            l_color, l_color };

                    l_textBackground.layer = ( 1 );
                }

                // Text
                {
                    const uint16_t l_layer = ( 2 );
                    const uint8_t l_alpha = 0xFF;
                    const uint16_t l_shade = 0xFF;
                    const uint16_t l_shade2 = 0xFF;
                    const uint8_t l_fontSize = 13;
                    const uint8_t l_textBackgroundLeftRightPadding = 15;
                    const uint8_t l_textBackgroundTopBottomPadding = 7;
                    const int32_t l_leftMargin =
                        ( 50 + l_textBackgroundLeftRightPadding );
                    const uint8_t l_rowTopMargin =
                        ( 6 + l_textBackgroundTopBottomPadding );
                    uint16_t l_index = 0;

                    for ( const auto& _item : g_jsonControlsKeyboard.items() ) {
                        const int32_t l_rowY =
                            ( 50 + l_overlayY +
                              ( int32_t )( ( l_index * l_fontSize ) +
                                           ( l_index * l_rowTopMargin ) ) );
                        std::string l_valueContent =
                            _item.value().template get< std::string >();

                        const uint32_t l_maxValueWidth = 325;

                        if ( l_valueContent.length() >
                             ( l_maxValueWidth / l_fontSize ) ) {
                            l_valueContent.resize(
                                ( l_maxValueWidth / l_fontSize ) - 3 );
                            std::fill_n( l_valueContent.rbegin(), 3, '.' );
                        }

                        const uint32_t l_valueWidth =
                            ( l_valueContent.length() * l_fontSize );

                        std::string l_keyContent = "";

                        if ( g_keyboardLayout.find( std::stoi(
                                 _item.key() ) ) != g_keyboardLayout.end() ) {
                            l_keyContent = ( g_keyboardLayout.at(
                                std::stoi( _item.key() ) ) );
                        }

                        const uint32_t l_keyWidth =
                            ( l_keyContent.length() * l_fontSize );
                        const int32_t l_keyX =
                            ( *( int32_t* )SCREEN_WIDTH - l_keyWidth -
                              ( l_leftMargin * 2 ) );

                        l_texts.push_back( { l_layer,
                                             l_alpha,
                                             l_shade,
                                             l_shade2,
                                             { l_leftMargin, l_rowY },
                                             { l_fontSize, l_fontSize },
                                             l_valueContent } );

                        l_textBackground.size = {
                            ( l_valueWidth +
                              ( l_textBackgroundLeftRightPadding * 2 ) ),
                            ( l_fontSize +
                              ( l_textBackgroundTopBottomPadding * 2 ) ) };
                        l_textBackground.coordinates = {
                            ( l_leftMargin - l_textBackgroundLeftRightPadding ),
                            ( l_rowY - l_textBackgroundTopBottomPadding ) };

                        if ( l_index == g_menuCursorIndex ) {
                            struct rectangle l_textBackgroundSelected =
                                l_textBackground;

                            const uint8_t l_alpha = 0xFF;
                            const uint8_t l_red = 0xFF;
                            const uint8_t l_green = 0;
                            const uint8_t l_blue = 0;
                            uint32_t l_color = 0;

                            _useCallback( "native$getColorForRectangle", 5,
                                          &l_alpha, &l_red, &l_green, &l_blue,
                                          &l_color );

                            l_textBackgroundSelected.colorsForRectangle = {
                                l_color, l_color, l_color, l_color };

                            l_rectangles.push_back( l_textBackgroundSelected );

                        } else {
                            l_rectangles.push_back( l_textBackground );
                        }

                        l_texts.push_back( { l_layer,
                                             l_alpha,
                                             l_shade,
                                             l_shade2,
                                             { l_keyX, l_rowY },
                                             { l_fontSize, l_fontSize },
                                             l_keyContent } );

                        l_textBackground.size = {
                            ( l_keyWidth +
                              ( l_textBackgroundLeftRightPadding * 2 ) ),
                            ( l_fontSize +
                              ( l_textBackgroundTopBottomPadding * 2 ) ) };
                        l_textBackground.coordinates = {
                            ( l_keyX - l_textBackgroundLeftRightPadding ),
                            ( l_rowY - l_textBackgroundTopBottomPadding ) };

                        if ( l_index == g_menuCursorIndex ) {
                            struct rectangle l_textBackgroundSelected =
                                l_textBackground;

                            const uint8_t l_alpha = 0xFF;
                            const uint8_t l_red = 0;
                            const uint8_t l_green = static_cast< uint8_t >(
                                0x59 * ( g_activeFlagsKeyboard &
                                         OVERLAY_IS_MAPPING_KEY ) );
                            const uint8_t l_blue = 0xFF;
                            uint32_t l_color = 0;

                            _useCallback( "native$getColorForRectangle", 5,
                                          &l_alpha, &l_red, &l_green, &l_blue,
                                          &l_color );

                            l_textBackgroundSelected.colorsForRectangle = {
                                l_color, l_color, l_color, l_color };

                            l_rectangles.push_back( l_textBackgroundSelected );

                        } else {
                            l_rectangles.push_back( l_textBackground );
                        }

                        l_index++;
                    }
                }
            }
        }

        // Render
        {
            // Background
            {
                for ( const struct rectangle _rectangle : l_rectangles ) {
                    _useCallback(
                        "native$drawRectangle", 9, &_rectangle.coordinates.x,
                        &_rectangle.coordinates.y, &_rectangle.size.width,
                        &_rectangle.size.height,
                        &_rectangle.colorsForRectangle.a,
                        &_rectangle.colorsForRectangle.b,
                        &_rectangle.colorsForRectangle.c,
                        &_rectangle.colorsForRectangle.d, &_rectangle.layer );
                }
            }

            // UI
            {
                // Text
                {
                    uintptr_t* l_fontAddress = ( uintptr_t* )FONT2;
                    uint32_t l_letterSpacing = 0;
                    char* l_out = 0;

                    for ( const struct text _text : l_texts ) {
                        _useCallback( "native$drawText", 12, &_text.size.width,
                                      &_text.size.height, &_text.coordinates.x,
                                      &_text.coordinates.y, &_text.content,
                                      &_text.alpha, &_text.shade, &_text.shade2,
                                      l_fontAddress, &l_letterSpacing,
                                      &_text.layer, &l_out );
                    }
                }
            }
        }

    } else {
        l_animationNeeded = true;
    }

    return ( 0 );
}
