#include "addon_callbacks.hpp"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define INITGUID
// Need at least version 8
#define DIRECTINPUT_VERSION 0x0800

#include <dinput.h>
// #include <dinputd.h>
#include <initguid.h>

#include <cstdint>
#include <set>

#include "_useCallback.h"
#include "button_t.h"
#include "controls_parse.hpp"
#include "d3d9.h"
#include "d3dx9.h"
#include "direction_t.h"
#include "native.hpp"
#include "player_t.h"

namespace {

const std::map< uint32_t, std::string > g_joystickLayout = {
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

uint32_t g_activeFlagsOverlay = 0;
bool g_disableMenu = false;
LPDIRECTINPUT8 g_pDI = NULL;
LPDIRECTINPUTDEVICE8 g_pJoystick = NULL;
HWND g_hFocusWindow = NULL;
uint32_t g_framesPassed = 0;
uint16_t g_menuCursorIndex = 0;

int32_t __attribute__( ( stdcall ) ) enumJoysticks( LPCDIDEVICEINSTANCE lpddi,
                                                    void* pvRef ) {
    static size_t l_index = 0;

    ( ( std::vector< DIDEVICEINSTANCE >* )pvRef )->push_back( *lpddi );

    LPDIRECTINPUTDEVICE8 l_tempDevice = NULL;

    g_pDI->CreateDevice( lpddi->guidInstance, &l_tempDevice, NULL );

    // Query for the actual joystick device to use
    IDirectInputDevice8_QueryInterface( l_tempDevice, IID_IDirectInputDevice8,
                                        ( void** )&g_pJoystick );

    if ( g_pJoystick ) {
        {
            const char l_message[] = "g_pJoystick\n";

            _useCallback( "log$transaction$query", 2, l_message,
                          sizeof( l_message ) );
        }

    } else {
        {
            const char l_message[] = "!g_pJoystick\n";

            _useCallback( "log$transaction$query", 2, l_message,
                          sizeof( l_message ) );
        }
    }

    // We no longer need the original DirectInput device pointer
    IDirectInputDevice8_Release( l_tempDevice );

    l_index++;

    return ( DIENUM_CONTINUE );
}

} // namespace

useCallbackFunction_t g_useCallback = NULL;

extern "C" uint16_t __declspec( dllexport ) mainLoop$newFrame(
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

    l_returnValue = _useCallback( "joystick$getInput$end", 2,
                                  &l_activeMappedKeys, &l_activeKeys );
    l_returnValue =
        _useCallback( "joystick$applyInput", 1, &l_activeMappedKeys );

    return ( l_returnValue );
}

extern "C" uint16_t __declspec( dllexport ) joystick$applyInput(
    void** _callbackArguments ) {
    uint16_t l_returnValue = 0;
    std::set< std::string >* _activeMappedKeys =
        ( std::set< std::string >* )_callbackArguments[ 0 ];
    direction_t l_direction = NEUTRAL_DIRECTION;
    button_t l_buttons = NEUTRAL_BUTTON;
    player_t l_localPlayer = FIRST;

    l_returnValue = _useCallback( "joystick$applyInput$begin", 2,
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

#if 0
    {
        if ( g_framesPassed >= 30 ) {
            if ( _activeMappedKeys->find( "ToggleOverlay_KeyConfig_Native" ) !=
                 _activeMappedKeys->end() ) {
                static bool l_wasOverlayToggled = false;
                const bool l_isOverlayToggled =
                    _useCallback( "overlay$getState" );

                g_framesPassed = 0;
            }
        }
    }
#endif

    {
        if ( !( g_activeFlagsOverlay & SHOW_NATIVE ) ) {
            l_returnValue = _useCallback( "game$applyInput", 3, &l_buttons,
                                          &l_direction, &l_localPlayer );
        }
    }

    l_returnValue =
        _useCallback( "joystick$applyInput$end", 1, _activeMappedKeys );

    return ( l_returnValue );
}

extern "C" uint16_t __declspec( dllexport ) gameMode$versus(
    void** _callbackArguments ) {
    g_disableMenu = true;

    return ( 0 );
}

extern "C" uint16_t __declspec( dllexport ) IDirect3D9Ex$CreateDevice(
    void** _callbackArguments ) {
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

    {
        const char l_message[] = "start\n";

        _useCallback( "log$transaction$query", 2, l_message,
                      sizeof( l_message ) );
    }

    DirectInput8Create( GetModuleHandle( NULL ), DIRECTINPUT_VERSION,
                        IID_IDirectInput8, ( VOID** )&g_pDI, NULL );

    IDirectInput8_Initialize( g_pDI, GetModuleHandle( NULL ),
                              DIRECTINPUT_VERSION );

    std::vector< DIDEVICEINSTANCE > activeJoysticks;

    IDirectInput8_EnumDevices( g_pDI, DI8DEVCLASS_GAMECTRL, enumJoysticks,
                               ( void* )&activeJoysticks, DIEDFL_ATTACHEDONLY );

    if ( !g_pJoystick ) {
        {
            const char l_message[] = "!g_pJoystick 2\n";

            _useCallback( "log$transaction$query", 2, l_message,
                          sizeof( l_message ) );
        }

        goto EXIT;
    }

    {
        g_pJoystick->SetCooperativeLevel(
            g_hFocusWindow,
            DISCL_EXCLUSIVE | DISCL_BACKGROUND // DISCL_NONEXCLUSIVE
        );

        g_pJoystick->SetDataFormat( &c_dfDIJoystick2 );

#if 0
        DIDEVCAPS ddc;
        ddc.dwSize = sizeof ( ddc );

        // Get the joystick capabilities
        result = IDirectInputDevice8_GetCapabilities ( device, &ddc );
        if ( FAILED ( result ) )
        {
            LOG ( "IDirectInputDevice8_GetCapabilities failed: 0x%08x", result );
            return;
        }

        info.device = device;
        info.numHats = min<uint64_t> ( MAX_NUM_HATS, ddc.dwPOVs );
        info.numButtons = min<uint64_t> ( MAX_NUM_BUTTONS, ddc.dwButtons );

        g_pJoystick->EnumObjects(
                EnumObjectsCallback,
                ( void* )g_hFocusWindow,
                DIDFT_AXIS // DIDFT_RELAXIS | DIDFT_PSHBUTTON
                );
#endif
    }

EXIT: {
    const char l_message[] = "end\n";

    _useCallback( "log$transaction$query", 2, l_message, sizeof( l_message ) );
}

    return ( 0 );
}

extern "C" uint16_t __declspec( dllexport ) joystick$getInput$end(
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
                if ( g_joystickLayout.find( *( _activeKeys->begin() ) ) !=
                     g_joystickLayout.end() ) {
                    uint16_t l_index = 0;

                    for ( const auto& _item : g_jsonControlsJoystick.items() ) {
                        if ( l_index == g_menuCursorIndex ) {
                            g_jsonControlsJoystick[ std::to_string(
                                *( _activeKeys->begin() ) ) ] =
                                _item.value().template get< std::string >();

                            g_jsonControlsJoystick.erase( _item.key() );

                            const std::string l_controlsConfigFileName =
                                ( std::string(
                                      CONTROLS_PREFERENCES_FILE_NAME ) +
                                  std::string( "." ) +
                                  std::string(
                                      CONTROLS_PREFERENCES_FILE_EXTENSION ) );

                            json l_jsonControls = {
                                { "joystick", g_jsonControlsJoystick },
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

                g_menuCursorIndex = ( g_jsonControlsJoystick.size() - 1 );

                g_activeFlagsOverlay &= ~OVERLAY_IS_MAPPING_KEY;

                g_framesPassed = 0;
            }

        } else if ( g_activeFlagsOverlay & SHOW_NATIVE ) {
            std::set< std::string >* _activeMappedKeys =
                ( std::set< std::string >* )_callbackArguments[ 0 ];

            if ( _activeMappedKeys->find( "8" ) != _activeMappedKeys->end() ) {
                g_menuCursorIndex--;

                if ( g_menuCursorIndex >
                     ( g_jsonControlsJoystick.size() - 1 ) ) {
                    g_menuCursorIndex = ( g_jsonControlsJoystick.size() - 1 );
                }

                g_framesPassed = 0;
            } else if ( _activeMappedKeys->find( "2" ) !=
                        _activeMappedKeys->end() ) {
                g_menuCursorIndex++;

                if ( g_menuCursorIndex >
                     ( g_jsonControlsJoystick.size() - 1 ) ) {
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

extern "C" uint16_t __declspec( dllexport ) extraDrawCallback(
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

                    for ( const auto& _item : g_jsonControlsJoystick.items() ) {
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

                        if ( g_joystickLayout.find( std::stoi(
                                 _item.key() ) ) != g_joystickLayout.end() ) {
                            l_keyContent = ( g_joystickLayout.at(
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
