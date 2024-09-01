#include "addon_callbacks.h"

#include <stdbool.h>
#include <stdint.h>

#include "_useCallback.h"
#include "button_t.h"
#include "controls_parse.h"
#include "d3d9.h"
#include "d3dx9.h"
#include "direction_t.h"
#include "player_t.h"
#include "stdfunc.h"

static const size_t g_keyboardLayoutKeys[] = {
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

static const char* g_keyboardLayoutValues[] = {
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

    _useCallbackInitialize();

    if ( _useCallback( "core$getSettingsContentByLabel", &g_settings,
                       "keyboard" ) != 0 ) {
        _useCallback( "core$readSettingsFromString", DEFAULT_SETTINGS );

        _useCallback( "core$getSettingsContentByLabel", &g_settings,
                      "keyboard" );
    }

    return ( 0 );
}

uint16_t __declspec( dllexport ) mainLoop$newFrame(
    void** _callbackArguments ) {
    uint16_t l_returnValue = 0;

    char** l_activeMappedKeys = ( char** )createArray( sizeof( char* ) );

    if ( ( GetActiveWindow() != g_hFocusWindow ) ||
         ( g_hFocusWindow == NULL ) ) {
        l_returnValue =
            _useCallback( "keyboard$applyInput", &l_activeMappedKeys );

        return ( l_returnValue );
    }

    char** l_activeKeys = ( char** )createArray( sizeof( char* ) );

#define KEYS_TOTAL ( 0xFE + 1 )

    for ( uint8_t _index = 0x0D; _index < KEYS_TOTAL; _index++ ) {
        if ( ( _index != VK_MENU ) && ( _index != VK_SHIFT ) &&
             ( _index != VK_CONTROL ) ) {
            if ( GetKeyState( _index ) & 0x8000 ) {
                const size_t l_keyboardLayoutKeysLength =
                    ( sizeof( g_keyboardLayoutKeys ) /
                      sizeof( g_keyboardLayoutKeys[ 0 ] ) );

                if ( contains( g_keyboardLayoutKeys, l_keyboardLayoutKeysLength,
                               _index ) ) {
                    const char* l_keyboardLayoutValue =
                        g_keyboardLayoutValues[ findInArray(
                            g_keyboardLayoutKeys, l_keyboardLayoutKeysLength,
                            _index ) ];

                    ssize_t l_mappedButtonKeyIndex = -1;

                    {
                        const size_t l_mappedButtonKeysLength =
                            ( size_t )( g_settings[ 0 ][ 0 ] );

                        for ( size_t _mappedButtonKeyIndex = 1;
                              _mappedButtonKeyIndex < l_mappedButtonKeysLength;
                              _mappedButtonKeyIndex++ ) {
                            const char* l_value =
                                g_settings[ _mappedButtonKeyIndex ][ 1 ];

                            if ( strcmp( l_value, l_keyboardLayoutValue ) ==
                                 0 ) {
                                l_mappedButtonKeyIndex = _mappedButtonKeyIndex;

                                break;
                            }
                        }
                    }

                    if ( l_mappedButtonKeyIndex >= 0 ) {
                        insertIntoArray(
                            ( void*** )( &l_activeMappedKeys ),
                            ( void* )( g_settings[ l_mappedButtonKeyIndex ]
                                                 [ 0 ] ),
                            sizeof( l_activeMappedKeys[ 0 ] ) );

#if D_PRINT_KEY
                        _useCallback( "log$transaction$query", "mapped: " );
                        _useCallback(
                            "log$transaction$query",
                            l_activeMappedKeys
                                [ ( ( size_t )( l_activeMappedKeys[ 0 ] ) ) -
                                  1 ] );
                        _useCallback( "log$transaction$query", "\n" );
                        _useCallback( "log$transaction$commit" );
#endif

                    } else {
                        insertIntoArray( ( void*** )( &l_activeKeys ),
                                         ( void* )( l_keyboardLayoutValue ),
                                         sizeof( l_activeKeys[ 0 ] ) );

#if D_PRINT_KEY
                        _useCallback( "log$transaction$query", "not mapped: " );
                        _useCallback(
                            "log$transaction$query",
                            l_activeKeys[ ( ( size_t )( l_activeKeys[ 0 ] ) ) -
                                          1 ] );
                        _useCallback( "log$transaction$query", "\n" );
                        _useCallback( "log$transaction$commit" );
#endif
                    }
                }
            }
        }
    }

#undef KEY_NUMBER

    l_returnValue = _useCallback( "keyboard$getInput$end", &l_activeMappedKeys,
                                  &l_activeKeys );
    l_returnValue = _useCallback( "keyboard$applyInput", &l_activeMappedKeys );

    free( l_activeMappedKeys );
    free( l_activeKeys );

    return ( l_returnValue );
}

uint16_t __declspec( dllexport ) keyboard$applyInput(
    void** _callbackArguments ) {
    uint16_t l_returnValue = 0;

    char*** _activeMappedKeys = ( char*** )_callbackArguments[ 0 ];
    size_t l_activeMappedKeysLength =
        ( ( size_t )( ( *_activeMappedKeys )[ 0 ] ) - 1 );
    direction_t l_direction = NEUTRAL_DIRECTION;
    button_t l_buttons = NEUTRAL_BUTTON;
    player_t l_localPlayer = FIRST;

    _useCallback( "keyboard$applyInput$begin", _activeMappedKeys,
                  &l_localPlayer );

    // Direction
    if ( l_activeMappedKeysLength ) {
        const char* l_directionsKeys[] = { "2", "4", "6", "8" };
        const size_t l_directionsKeysLength =
            ( sizeof( l_directionsKeys ) / sizeof( l_directionsKeys[ 0 ] ) );
        const int16_t l_directionsValues[][ 2 ] = {
            { -1, -1 }, // 1
            { 0, -1 },  // 2
            { 1, -1 },  // 3
            { -1, 0 },  // 4
            { 0, 0 },   // 5
            { 1, 0 },   // 6
            { -1, 1 },  // 7
            { 0, 1 },   // 8
            { 1, 1 }    // 9
        };
        const size_t l_directionsValuesLength =
            ( sizeof( l_directionsValues ) /
              sizeof( l_directionsValues[ 0 ] ) );
        int16_t l_pressedDirectionValue[ 2 ] = { 0, 0 };
        const size_t l_pressedDirectionValueLength =
            ( sizeof( l_pressedDirectionValue ) /
              sizeof( l_pressedDirectionValue[ 0 ] ) );

        for ( size_t _index = 1; _index < ( l_directionsKeysLength + 1 );
              _index++ ) {
            if ( _containsString( *_activeMappedKeys,
                                  l_directionsKeys[ _index ] ) ) {
                _useCallback( "log$transaction$query", "\n" );
                _useCallback( "log$transaction$commit" );
                const int16_t* l_directionValue =
                    l_directionsValues[ ( _index * 2 ) - 1 ];
                _useCallback( "log$transaction$query", "\n" );
                _useCallback( "log$transaction$commit" );

                l_pressedDirectionValue[ 0 ] += l_directionValue[ 0 ];
                _useCallback( "log$transaction$query", "\n" );
                _useCallback( "log$transaction$commit" );
                l_pressedDirectionValue[ 1 ] += l_directionValue[ 1 ];
                _useCallback( "log$transaction$query", "\n" );
                _useCallback( "log$transaction$commit" );

                l_activeMappedKeysLength--;

                if ( !l_activeMappedKeysLength ) {
                    break;
                }
            }
        }

#if 0
        if ( containsArray( l_directionsValues, l_directionsValuesLength,
                            l_pressedDirectionValue,
                            l_pressedDirectionValueLength ) ) {
            l_direction =
                ( findArrayInArray(
                      l_directionsValues, l_directionsValuesLength,
                      l_pressedDirectionValue, l_pressedDirectionValueLength ) +
                  1 );
        }
#endif
    }

    // Button
    if ( l_activeMappedKeysLength ) {
        const char* l_tempKeys[] = { "A",  "B",   "C",   "D",    "E",
                                     "AB", "FN1", "FN2", "START" };

        const button_t l_tempValues[] = {
            ( ( button_t )( ( uint16_t )( A ) | ( uint16_t )( CONFIRM ) ) ),
            ( ( button_t )( ( uint16_t )( B ) | ( uint16_t )( CANCEL ) ) ),
            C,
            D,
            E,
            AB,
            FN1,
            FN2,
            START };

        for ( size_t _index = 0;
              ( sizeof( l_tempKeys ) / sizeof( l_tempKeys[ 0 ] ) ); _index++ ) {
            const char* l_key = l_tempKeys[ _index ];

            if ( _containsString( *_activeMappedKeys, l_key ) ) {
                const button_t l_value = l_tempValues[ _index ];
                l_buttons = ( button_t )( ( uint16_t )( l_buttons ) |
                                          ( uint16_t )( l_value ) );

                l_activeMappedKeysLength--;

                if ( !l_activeMappedKeysLength ) {
                    break;
                }
            }
        }
    }

    _useCallback( "keyboard$applyInput$end", &l_buttons, &l_direction,
                  &l_localPlayer );

    l_returnValue = _useCallback( "game$applyInput", &l_buttons, &l_direction,
                                  &l_localPlayer );

    return ( l_returnValue );
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

                _useCallback( "native$getColorForRectangle", &l_alpha,
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

                    _useCallback( "native$getColorForRectangle", &l_alpha,
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

                            _useCallback( "native$getColorForRectangle",
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

                            _useCallback( "native$getColorForRectangle",
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
                        "native$drawRectangle", &_rectangle.coordinates.x,
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
                        _useCallback( "native$drawText", &_text.size.width,
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
