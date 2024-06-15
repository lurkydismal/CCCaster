#include "addon_callbacks.hpp"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <cstdint>
#include <set>

// #include <fmt/core.h>
// #include <icecream/icecream.hpp>

#include "_useCallback.hpp"
#include "button_input.h"
#include "controls_parse.hpp"
#include "d3d9.h"
#include "d3dx9.h"
#include "direction_input.h"
#include "imgui.hpp"
#include "native.hpp"
#include "player_t.h"

#pragma comment( lib, "user32.lib" )

namespace {

typedef struct {
    std::string label;
    uint32_t virtualKeyCode;
    uint32_t offset;
    uint32_t width;
} keyLayout_t;

uint32_t g_activeFlagsKeyboard = 0;
bool g_disableMenu = false;
const std::vector< std::vector< keyLayout_t > > l_keyboardLayout = {
    {
        { "Esc", VK_ESCAPE, 18, 0 },
        { "F1", VK_F1, 18, 0 },
        { "F2", VK_F2, 0, 0 },
        { "F3", VK_F3, 0, 0 },
        { "F4", VK_F4, 0, 0 },
        { "F5", VK_F5, 24, 0 },
        { "F6", VK_F6, 0, 0 },
        { "F7", VK_F7, 0, 0 },
        { "F8", VK_F8, 0, 0 },
        { "F9", VK_F9, 24, 0 },
        { "F10", VK_F10, 0, 0 },
        { "F11", VK_F11, 0, 0 },
        { "F12", VK_F12, 0, 0 },
        { "Pause", VK_PAUSE, 120, 0 },
    },
    {
        { "~", VK_OEM_3, 0, 0 },
        { "1", 0x31, 0, 0 },
        { "2", 0x32, 0, 0 },
        { "3", 0x33, 0, 0 },
        { "4", 0x34, 0, 0 },
        { "5", 0x35, 0, 0 },
        { "6", 0x36, 0, 0 },
        { "7", 0x37, 0, 0 },
        { "8", 0x38, 0, 0 },
        { "9", 0x39, 0, 0 },
        { "0", 0x30, 0, 0 },
        { "-", VK_OEM_MINUS, 0, 0 },
        { "+", VK_OEM_PLUS, 0, 0 },
        { "Backspace", VK_BACK, 0, 80 },
        { "Insert", VK_INSERT, 24, 0 },
        { "Home", VK_HOME, 0, 0 },
        { "PgUp", VK_PRIOR, 0, 0 },
    },
    {
        { "Tab", VK_TAB, 0, 60 },
        { "Q", 0x51, 0, 0 },
        { "W", 0x57, 0, 0 },
        { "E", 0x45, 0, 0 },
        { "R", 0x52, 0, 0 },
        { "T", 0x54, 0, 0 },
        { "Y", 0x59, 0, 0 },
        { "U", 0x55, 0, 0 },
        { "I", 0x49, 0, 0 },
        { "O", 0x4F, 0, 0 },
        { "P", 0x50, 0, 0 },
        { "[", VK_OEM_4, 0, 0 },
        { "]", VK_OEM_6, 0, 0 },
        { "|", VK_OEM_5, 0, 60 },
        { "Del", VK_DELETE, 24, 0 },
        { "End", VK_END, 0, 0 },
        { "PgDn", VK_NEXT, 0, 0 },
    },
    {
        { "Caps Lock", VK_CAPITAL, 0, 80 },
        { "A", 0x41, 0, 0 },
        { "S", 0x53, 0, 0 },
        { "D", 0x44, 0, 0 },
        { "F", 0x46, 0, 0 },
        { "G", 0x47, 0, 0 },
        { "H", 0x48, 0, 0 },
        { "J", 0x4A, 0, 0 },
        { "K", 0x4B, 0, 0 },
        { "L", 0x4C, 0, 0 },
        { ";", VK_OEM_1, 0, 0 },
        { "\'", VK_OEM_7, 0, 0 },
        { "Enter", VK_RETURN, 0, 84 },
    },
    {
        { "L.Shift", VK_LSHIFT, 0, 104 },
        { "Z", 0x5A, 0, 0 },
        { "X", 0x58, 0, 0 },
        { "C", 0x43, 0, 0 },
        { "V", 0x56, 0, 0 },
        { "B", 0x42, 0, 0 },
        { "N", 0x4E, 0, 0 },
        { "M", 0x4D, 0, 0 },
        { ",", VK_OEM_COMMA, 0, 0 },
        { ".", VK_OEM_PERIOD, 0, 0 },
        { "/", VK_OEM_2, 0, 0 },
        { "R.Shift", VK_RSHIFT, 0, 104 },
        { "Up", VK_UP, 68, 0 },
    },
    { { "L.Ctrl", VK_LCONTROL, 0, 60 },
      { "L.Alt", VK_LMENU, 68, 60 },
      { "Space", VK_SPACE, 0, 260 },
      { "R.Alt", VK_RMENU, 0, 60 },
      { "R.Ctrl", VK_RCONTROL, 68, 60 },
      { "Left", VK_LEFT, 24, 0 },
      { "Down", VK_DOWN, 0, 0 },
      { "Right", VK_RIGHT, 0, 0 } } };

HWND g_hFocusWindow = NULL;

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

    std::set< std::string > l_activeMappedKeys;
    std::set< uint8_t > l_activeKeys;

#define KEYS_TOTAL ( 0xFE + 1 )

    for ( uint8_t _index = 0x0D; _index < KEYS_TOTAL; _index++ ) {
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
    std::set< std::string > _activeMappedKeys =
        *( std::set< std::string >* )_callbackArguments[ 0 ];
    direction_t l_direction = NEUTRAL_DIRECTION;
    button_t l_buttons = NEUTRAL_BUTTON;

    l_returnValue =
        _useCallback( "keyboard$applyInput$begin", 1, _activeMappedKeys );

    if ( _activeMappedKeys.find( "8" ) != _activeMappedKeys.end() ) {
        l_direction = UP;
    }

    if ( _activeMappedKeys.find( "2" ) != _activeMappedKeys.end() ) {
        l_direction = DOWN;
    }

    if ( _activeMappedKeys.find( "4" ) != _activeMappedKeys.end() ) {
        l_direction = ( l_direction )
                          ? static_cast< direction_t >(
                                --*reinterpret_cast< uint8_t* >( l_direction ) )
                          : LEFT;
    }

    if ( _activeMappedKeys.find( "6" ) != _activeMappedKeys.end() ) {
        l_direction = ( l_direction )
                          ? static_cast< direction_t >(
                                ++*reinterpret_cast< uint8_t* >( l_direction ) )
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
        if ( _activeMappedKeys.find( _button.first ) !=
             _activeMappedKeys.end() ) {
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

    /*  {
        static uint32_t l_framesPassed = 0;

        if ( l_framesPassed >= 10 ) {
        l_framesPassed = 0;

        if ( GetKeyState(
        g_jsonControlsKeyboard[ "ToggleOverlay_KeyConfig" ] ) )
        { if ( _useCallback( "overlay$Toggle" ) ) { g_activeFlagsKeyboard |=
        SHOW_OVERLAY_KEY_CONFIG;
        }

        } else if ( GetKeyState(
        g_jsonControlsKeyboard
        [ "ToggleOverlay_KeyConfig_Native" ] ) ) {
    // if ( _useCallback( "overlay$Toggle" ) ) {
    g_activeFlagsKeyboard ^= SHOW_OVERLAY_KEY_CONFIG_NATIVE;
    // }
    }

    } else {
    l_framesPassed++;
    }
    }
    */
    {
        player_t l_localPlayer = FIRST;

        applyInput( l_buttons, l_direction, l_localPlayer );
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
    overlay$beforeDraw$ImGui( void** _callbackArguments ) {
    static uint8_t l_mapping;

    if ( g_activeFlagsKeyboard & SHOW_OVERLAY_KEY_CONFIG ) {
        for ( std::vector< keyLayout_t > const& _keyboardLayoutLine :
              l_keyboardLayout ) {
            uint8_t l_keyIndex = 0;

            _useCallback( "ImGui$BeginGroup" );

            for ( keyLayout_t const& _keyLayout : _keyboardLayoutLine ) {
                const float l_spacingBefore = _keyLayout.offset + 4.f;

                const float l_width =
                    ( _keyLayout.width ) ? ( _keyLayout.width ) : ( 40 );

                if ( l_keyIndex ) {
                    float l_offsetFromStartX = 0.f;

                    _useCallback( "ImGui$SameLine", 2, &l_offsetFromStartX,
                                  &l_spacingBefore );

                } else {
                    if ( l_spacingBefore >= 1.f ) {
                        _useCallback( "ImGui$Indent", 1, &l_spacingBefore );
                    }
                }

                const char* l_buttonLabel = _keyLayout.label.c_str();
                const ImVec2 l_buttonSize = ImVec2( l_width, 40 );

                uint16_t l_result = _useCallback(
                    "ImGui$Button", 2, &l_buttonLabel, &l_buttonSize );

                if ( ( l_result ) && ( l_result != ENODATA ) ) {
                    l_mapping = _keyLayout.virtualKeyCode;

                    g_activeFlagsKeyboard &= ~SHOW_OVERLAY_KEY_CONFIG;
                    g_activeFlagsKeyboard |= SHOW_OVERLAY_KEY_CONFIG_ACTION;
                }

                l_keyIndex++;
            }

            _useCallback( "ImGui$EndGroup" );
        }
    }

    if ( g_activeFlagsKeyboard & SHOW_OVERLAY_KEY_CONFIG_ACTION ) {
        ImGuiID l_childId = 127;
        const ImVec2 l_childSize = ImVec2( 220, -1 );
        ImGuiChildFlags l_childFlags = ImGuiChildFlags_FrameStyle;
        ImGuiWindowFlags l_childWindowFlags = 0;

        _useCallback( "ImGui$BeginChild", 4, &l_childId, &l_childSize,
                      &l_childFlags, &l_childWindowFlags );

        for ( const auto& _mappedKey : g_jsonControlsKeyboard.items() ) {
            const char* l_selectableLabel = _mappedKey.key().c_str();
            const bool l_isSelected = true;
            const ImGuiSelectableFlags l_selectableFlags = 0;
            const ImVec2 l_selectableSize = ImVec2( 0, 0 );

            uint16_t l_result = _useCallback(
                "ImGui$Selectable", 4, &l_selectableLabel, &l_isSelected,
                &l_selectableFlags, &l_selectableSize );

            if ( ( l_result ) && ( l_result != ENODATA ) ) {
                const std::string l_action = _mappedKey.key();

                g_jsonControlsKeyboard[ l_action ] = l_mapping;

                // g_jsonControlsKeyboard.overwrite();

                g_activeFlagsKeyboard &= ~SHOW_OVERLAY_KEY_CONFIG_ACTION;
                g_activeFlagsKeyboard |= SHOW_OVERLAY_KEY_CONFIG;
            }
        }

        _useCallback( "ImGui$EndChild" );
    }

    return ( 0 );
}

extern "C" uint16_t __declspec( dllexport ) overlay$Toggle( void ) {
    g_activeFlagsKeyboard &= ~SHOW_OVERLAY_KEY_CONFIG;
    g_activeFlagsKeyboard &= ~SHOW_OVERLAY_KEY_CONFIG_ACTION;
    g_activeFlagsKeyboard &= ~SHOW_OVERLAY_KEY_CONFIG_NATIVE;

    return ( 0 );
}

extern "C" uint16_t __declspec( dllexport )
    extraDrawCallback( void** _callbackArguments ) {
    if ( g_activeFlagsKeyboard & SHOW_OVERLAY_KEY_CONFIG_NATIVE ) {
        struct background {
            colorsForRectangle_t colorsForRectangle;
            coordinates_t coordinates;
            struct size size;
        } l_background;

        // Logic
        {
            // Background
            {
                const uint8_t l_alpha = 0xff;
                const uint8_t l_red = 30;
                const uint8_t l_green = 30;
                const uint8_t l_blue = 0xff;
                uint32_t l_color = 0;

                _useCallback( "native$getColorForRectangle", 5, &l_alpha,
                              &l_red, &l_green, &l_blue, &l_color );

                l_background.colorsForRectangle = { l_color, l_color, l_color,
                                                    l_color };

                l_background.coordinates = { 0, 0 };
                l_background.size = { *( uint32_t* )SCREEN_WIDTH,
                                      ( 300 - 50 ) };
            }

            // UI
            {
                // Text
                {}

                // Sprite
                {}
            }
        }

        std::string l_test = "ttt";

        uint32_t l_width2 = 24;
        uint32_t l_height2 = 24;
        uint32_t l_x2 = ( 300 - 1 );
        uint32_t l_y2 = 50;
        const char* l_text = l_test.data();
        uint32_t l_alpha2 = 0xff;
        uint32_t l_shade = 0xff;
        uint32_t l_shade2 = 0x1f0;

        uint32_t l_width3 = 25;
        uintptr_t* l_textureAddress = ( uintptr_t* )BUTTON_SPRITE_TEX;
        uint32_t l_x3 = 500;
        uint32_t l_y3 = 250;
        uint32_t l_height3 = 25;
        uint32_t l_textureX = ( 0x19 * 6 );
        uint32_t l_textureY = 0;
        uint32_t l_textureWidth = 0x19;
        uint32_t l_textureHeight = 0x19;

        // Render
        {
            // Background
            {
                uint32_t l_layerIndex = 0x2ff;

                _useCallback(
                    "native$drawRectangle", 9, &l_background.coordinates.x,
                    &l_background.coordinates.y, &l_background.size.width,
                    &l_background.size.height,
                    &l_background.colorsForRectangle.a,
                    &l_background.colorsForRectangle.b,
                    &l_background.colorsForRectangle.c,
                    &l_background.colorsForRectangle.d, &l_layerIndex );
            }

            // UI
            {
                // Text
                {
                    uintptr_t* l_fontAddress = ( uintptr_t* )FONT2;
                    uint32_t l_letterSpacing = 0;
                    uint32_t l_layerIndex = 0;
                    char* l_out = 0;

                    _useCallback( "native$drawText", 12, &l_width2, &l_height2,
                                  &l_x2, &l_y2, &l_text, &l_alpha2, &l_shade,
                                  &l_shade2, l_fontAddress, &l_letterSpacing,
                                  &l_layerIndex, &l_out );
                }

                // Sprite
                {
                    uint32_t l_directXDevice = 0;
                    uint32_t l_flags = 0xFFFFFFFF;
                    uint32_t l_unknown = 0;
                    uint32_t l_layerIndex = 0x2cc;

                    _useCallback( "native$drawSprite", 13, &l_width3,
                                  &l_directXDevice, &*l_textureAddress, &l_x3,
                                  &l_y3, &l_height3, &l_textureX, &l_textureY,
                                  &l_textureWidth, &l_textureHeight, &l_flags,
                                  &l_unknown, &l_layerIndex );
                }
            }
        }
    }

    return ( 0 );
}
