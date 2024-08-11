#include "addon_callbacks.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <stdbool.h>
#include <stdint.h>

#include "_useCallback.h"
#include "button_t.h"
#include "controls_parse.h"
#include "d3d9.h"
#include "d3dx9.h"
#include "direction_t.h"
#include "native.h"
#include "player_t.h"

const size_t g_keyboardLayoutKeys[] = {
    VK_ESCAPE,     VK_F1,       VK_F2,     VK_F3,       VK_F4,
    VK_F5,         VK_F6,       VK_F7,     VK_F8,       VK_F9,
    VK_F10,        VK_F11,      VK_F12,    VK_PAUSE,    VK_OEM_3,
    0x31,          0x32,        0x33,      0x34,        0x35,
    0x36,          0x37,        0x38,      0x39,        0x30,
    VK_OEM_MINUS,  VK_OEM_PLUS, VK_BACK,   VK_INSERT,   VK_HOME,
    VK_PRIOR,      VK_TAB,      0x51,      0x57,        0x45,
    0x52,          0x54,        0x59,      0x55,        0x49,
    0x4F,          0x50,        VK_OEM_4,  VK_OEM_6,    VK_OEM_5,
    VK_DELETE,     VK_END,      VK_NEXT,   VK_CAPITAL,  0x41,
    0x53,          0x44,        0x46,      0x47,        0x48,
    0x4A,          0x4B,        0x4C,      VK_OEM_1,    VK_OEM_7,
    VK_RETURN,     VK_LSHIFT,   0x5A,      0x58,        0x43,
    0x56,          0x42,        0x4E,      0x4D,        VK_OEM_COMMA,
    VK_OEM_PERIOD, VK_OEM_2,    VK_RSHIFT, VK_UP,       VK_LCONTROL,
    VK_LMENU,      VK_SPACE,    VK_RMENU,  VK_RCONTROL, VK_LEFT,
    VK_DOWN,       VK_RIGHT };

const char* g_keyboardLayoutValues[] = {
    "Esc",    "F1",     "F2",      "F3",   "F4",     "F5",      "F6",
    "F7",     "F8",     "F9",      "F10",  "F11",    "F12",     "Pause",
    "~",      "1",      "2",       "3",    "4",      "5",       "6",
    "7",      "8",      "9",       "0",    "-",      "+",       "Backspace",
    "Insert", "Home",   "PgUp",    "Tab",  "Q",      "W",       "E",
    "R",      "T",      "Y",       "U",    "I",      "O",       "P",
    "[",      "]",      "|",       "Del",  "End",    "PgDn",    "Caps Lock",
    "A",      "S",      "D",       "F",    "G",      "H",       "J",
    "K",      "L",      ";",       "\'",   "Enter",  "L.Shift", "Z",
    "X",      "C",      "V",       "B",    "N",      "M",       ",",
    ".",      "/",      "R.Shift", "Up",   "L.Ctrl", "L.Alt",   "Space",
    "R.Alt",  "R.Ctrl", "Left",    "Down", "Right" };

static HWND g_hFocusWindow;
static uint32_t g_framesPassed = 0;
static uint32_t g_activeFlagsOverlay = 0;
static bool g_disableMenu = false;
static uint16_t g_menuCursorIndex = 0;

useCallbackFunction_t g_useCallback;
char*** g_settings;

uint16_t __declspec( dllexport ) IDirect3D9Ex$CreateDevice(
    void** _callbackArguments ) {
    HWND* _hFocusWindow = ( HWND* )_callbackArguments[ 2 ];
    D3DPRESENT_PARAMETERS** _pPresentationParameters =
        ( D3DPRESENT_PARAMETERS** )_callbackArguments[ 4 ];

    g_hFocusWindow = ( *_hFocusWindow )
                         ? ( *_hFocusWindow )
                         : ( ( *_pPresentationParameters )->hDeviceWindow );

    void* l_statesDll = GetModuleHandleA( "states.dll" );

    if ( !l_statesDll ) {
        exit( 1 );
    }

    g_useCallback =
        ( useCallbackFunction_t )GetProcAddress( l_statesDll, "useCallback" );

    if ( !g_useCallback ) {
        exit( 1 );
    }

    if ( _useCallback( "core$getSettingsContentByLabel", &g_settings,
                       "keyboard" ) != 0 ) {
        const char l_defaultSettings[] =
            "[keyboard]\n"
            "8 = 38\n"
            "6 = 39\n"
            "2 = 40\n"
            "4 = 37\n"
            "A = 90\n"
            "B = 88\n"
            "C = 67\n"
            "D = 86\n"
            "E = 68\n"
            "AB = 83\n"
            "FN1 = 221\n"
            "FN2 = 82\n"
            "START = 84\n"
            "ToggleOverlay_KeyConfig_Native = 115\n";

        _useCallback( "core$readSettingsFromString", l_defaultSettings );
    }

    return ( 0 );
}

uint16_t __declspec( dllexport ) mainLoop$newFrame(
    void** _callbackArguments ) {
    uint16_t l_returnValue = 0;

    if ( g_framesPassed < ( UINT32_MAX - 1 ) ) {
        g_framesPassed++;
    }

    if ( ( GetActiveWindow() != g_hFocusWindow ) ||
         ( g_hFocusWindow == NULL ) ) {
        return ( l_returnValue );
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

uint16_t __declspec( dllexport ) keyboard$applyInput(
    void** _callbackArguments ) {
    uint16_t l_returnValue = 0;
    std::set< std::string >* _activeMappedKeys =
        ( std::set< std::string >* )_callbackArguments[ 0 ];
    direction_t l_direction = NEUTRAL_DIRECTION;
    button_t l_buttons = NEUTRAL_BUTTON;
    player_t l_localPlayer = FIRST;

    l_returnValue = _useCallback( "keyboard$applyInput$begin", 2,
                                  _activeMappedKeys, &l_localPlayer );

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
                static bool l_wasOverlayToggled = false;
                const bool l_isOverlayToggled =
                    _useCallback( "overlay$getState" );

                if ( ( !l_isOverlayToggled ) && ( !l_wasOverlayToggled ) ) {
                    _useCallback( "overlay$toggle" );
                    l_wasOverlayToggled = true;

                    g_activeFlagsOverlay = SHOW_NATIVE;

                } else if ( ( l_isOverlayToggled ) &&
                            ( l_wasOverlayToggled ) ) {
                    _useCallback( "overlay$toggle" );
                    l_wasOverlayToggled = false;

                    g_activeFlagsOverlay &= ~SHOW_NATIVE;
                }

                g_framesPassed = 0;
            }
        }
    }

    {
        if ( !( g_activeFlagsOverlay & SHOW_NATIVE ) ) {
            l_returnValue = _useCallback( "game$applyInput", 3, &l_buttons,
                                          &l_direction, &l_localPlayer );
        }
    }

    l_returnValue =
        _useCallback( "keyboard$applyInput$end", 1, _activeMappedKeys );

    return ( l_returnValue );
}

uint16_t __declspec( dllexport ) gameMode$versus(
    void** _callbackArguments ) {
    g_disableMenu = true;

    return ( 0 );
}

#if 0
uint16_t __declspec( dllexport ) keyboard$getInput$end(
    void** _callbackArguments ) {
    if ( g_framesPassed > 7 ) {
        if ( g_activeFlagsOverlay & OVERLAY_IS_MAPPING_KEY ) {
            std::set< std::string >* _activeMappedKeys =
                ( std::set< std::string >* )_callbackArguments[ 0 ];
            std::set< uint8_t >* _activeKeys =
                ( std::set< uint8_t >* )_callbackArguments[ 1 ];

            if ( _activeMappedKeys->find( "B" ) != _activeMappedKeys->end() ) {
                g_activeFlagsOverlay &= ~OVERLAY_IS_MAPPING_KEY;

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

                g_activeFlagsOverlay &= ~OVERLAY_IS_MAPPING_KEY;

                g_framesPassed = 0;
            }

        } else if ( g_activeFlagsOverlay & SHOW_NATIVE ) {
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
                g_activeFlagsOverlay |= OVERLAY_IS_MAPPING_KEY;

                g_framesPassed = 0;

            } else if ( _activeMappedKeys->find( "B" ) !=
                        _activeMappedKeys->end() ) {
                g_activeFlagsOverlay &= ~OVERLAY_IS_MAPPING_KEY;

                g_framesPassed = 0;
            }
        }
    }

    return ( 0 );
}

uint16_t __declspec( dllexport ) extraDrawCallback(
    void** _callbackArguments ) {
    uint16_t l_returnValue = 0;

    static bool l_animationNeeded = true;

    if ( g_activeFlagsOverlay & SHOW_NATIVE ) {
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
                    const uint8_t l_maxItemsPerPage = 15;
                    const uint16_t l_firstIndexToShow =
                        ( g_menuCursorIndex > ( l_maxItemsPerPage - 1 ) )
                            ? ( g_menuCursorIndex - ( l_maxItemsPerPage - 1 ) )
                            : ( 0 );
                    uint16_t l_index = 0;
                    uint16_t l_indexForShown = 0;

                    for ( const auto& _item : g_jsonControlsKeyboard.items() ) {
                        if ( l_index < l_firstIndexToShow ) {
                            l_index++;

                            continue;
                        }

                        if ( l_indexForShown >= l_maxItemsPerPage ) {
                            break;
                        }

                        const int32_t l_rowY =
                            ( 50 + l_overlayY +
                              ( int32_t )( ( l_indexForShown * l_fontSize ) +
                                           ( l_indexForShown *
                                             l_rowTopMargin ) ) );
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

                        if ( l_indexForShown ==
                             ( g_menuCursorIndex - l_firstIndexToShow ) ) {
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

                        if ( l_indexForShown ==
                             ( g_menuCursorIndex - l_firstIndexToShow ) ) {
                            struct rectangle l_textBackgroundSelected =
                                l_textBackground;

                            const uint8_t l_alpha = 0xFF;
                            const uint8_t l_red = 0;
                            const uint8_t l_green = static_cast< uint8_t >(
                                0x59 * ( g_activeFlagsOverlay &
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
                        l_indexForShown++;
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
#endif
