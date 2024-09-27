#include <omp.h>
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

#if 0
#define PRINT_KEY
#endif

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

uint16_t __declspec( dllexport ) gameMode$opening( void** _callbackArguments ) {
    static bool l_isNeedToRegisterOverlay = false;

    if ( !l_isNeedToRegisterOverlay ) {
        const uint16_t l_returnValue = _useCallback(
            "overlay$register", "keyboard", "F4", "[text]\ncontent=huh", NULL );

        if ( ( l_returnValue == 0 ) || ( l_returnValue == ENODATA ) ) {
            l_isNeedToRegisterOverlay = true;
        }
    }

    return ( 0 );
}

static bool checkKey( uint8_t _key ) {
    bool l_returnValue = false;

    {
        // Check Pressed 0x8000 ==
        //  0b1000_0000_0000_0000 16bit
        //  If the high-order bit is 1, the key is down; otherwise, it is up.
        //  If the low-order bit is 1, the key is toggled.
        if ( GetKeyState( _key ) & 0x8000 ) {
            l_returnValue = true;
        }
    }

    return ( l_returnValue );
}

static ssize_t findKeyInSettings( const char* _key ) {
    ssize_t l_returnValue = -1;

    const size_t l_mappedButtonKeysLength = arrayLength( g_settings );
    char*** l_mappedButtonKeysFirstElement =
        arrayFirstElementPointer( g_settings );
    char** const* l_mappedButtonKeysEnd =
        ( l_mappedButtonKeysFirstElement + l_mappedButtonKeysLength );

    for ( char*** _pair = l_mappedButtonKeysFirstElement;
          _pair != l_mappedButtonKeysEnd; _pair++ ) {
        const char* l_value = ( *_pair )[ 1 ];

        if ( strcmp( l_value, _key ) == 0 ) {
            l_returnValue = ( _pair - l_mappedButtonKeysFirstElement + 1 );

            break;
        }
    }

EXTI:
    return ( l_returnValue );
}

uint16_t __declspec( dllexport ) mainLoop$newFrame(
    void** _callbackArguments ) {
    uint16_t l_returnValue = 0;

    char** l_activeMappedKeys = ( char** )createArray( sizeof( char* ) );

    if ( ( GetActiveWindow() != g_hFocusWindow ) ||
         ( g_hFocusWindow == NULL ) ) {
        l_returnValue = _useCallback( "game$applyInput", &l_activeMappedKeys );

        return ( l_returnValue );
    }

    char** l_activeKeys = ( char** )createArray( sizeof( char* ) );

    const size_t l_keyboardLayoutKeysLength =
        ( sizeof( g_keyboardLayoutKeys ) /
          sizeof( g_keyboardLayoutKeys[ 0 ] ) );

    for ( uint8_t _keyIndex = 0; _keyIndex < l_keyboardLayoutKeysLength;
          _keyIndex++ ) {
        if ( !checkKey( g_keyboardLayoutKeys[ _keyIndex ] ) ) {
            continue;
        }

        char*** l_array;
        const char* l_key;

        const char* l_keyboardLayoutValue = g_keyboardLayoutValues[ _keyIndex ];
        ssize_t l_mappedButtonKeyIndex =
            findKeyInSettings( l_keyboardLayoutValue );

        if ( l_mappedButtonKeyIndex >= 0 ) {
            const char* l_activeMappedKey =
                g_settings[ l_mappedButtonKeyIndex ][ 0 ];

            l_array = &l_activeMappedKeys;
            l_key = l_activeMappedKey;

        } else {
            l_array = &l_activeKeys;
            l_key = l_keyboardLayoutValue;

#if defined( PRINT_KEY )
            _useCallback( "log$transaction$query", "not " );
#endif
        }

        insertIntoArray( ( void*** )( l_array ), ( void* )( l_key ) );

#if defined( PRINT_KEY )
        _useCallback( "log$transaction$query", "mapped: " );
        _useCallback( "log$transaction$query", l_key );
        _useCallback( "log$transaction$query", "\n" );
#endif
    }

    l_returnValue = _useCallback( "keyboard$getInput$end", &l_activeMappedKeys,
                                  &l_activeKeys );
    l_returnValue = _useCallback( "game$applyInput", &l_activeMappedKeys );

    free( l_activeMappedKeys );
    free( l_activeKeys );

    return ( l_returnValue );
}
