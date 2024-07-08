#include "addon_callbacks.hpp"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <winsock2.h>
#include <ws2tcpip.h>

#include <vector>
#include <set>

#include "player_t.h"
#include "_useCallback.hpp"
#include "d3d9.h"
#include "native.hpp"

#pragma comment( lib, "user32.lib" )
#pragma comment( lib, "ws2_32.lib" )
#pragma comment( lib, "mswsock.lib" )

useCallbackFunction_t g_useCallback = NULL;

namespace {

uint32_t g_framesPassed = 0;
WSADATA g_wsaData;
struct addrinfo g_hints;
struct addrinfo* g_result;
SOCKET g_connectSocket;
const player_t g_remotePlayer = SECOND;
HWND g_hFocusWindow = NULL;
uint32_t g_activeFlagsOverlay = 0;
std::string g_address = "0";
uint8_t g_type = 0;
uint8_t g_action = 0;

const std::vector<
    std::pair< std::string, std::pair< std::vector< std::string >, void* > > >
    g_overlayLayout = {
        { "Address", { { "" }, ( void* )&g_address } },
        { "Type",
          { { "Direct", "GGRS Rollback", "GGRS Direct" }, ( void* )&g_type } },
        { "Action",
          { { "Connect", "Host", "Spectate" }, ( void* )&g_action } } };

} // namespace

extern "C" uint16_t __declspec( dllexport ) mainLoop$newFrame(
    void** _callbackArguments ) {
    uint16_t l_returnValue = 0;

    if ( g_framesPassed < ( UINT32_MAX - 1 ) ) {
        g_framesPassed++;
    }

    return ( l_returnValue );
}
extern "C" uint16_t __declspec( dllexport ) gameMode$characterSelect(
    void** _callbackArguments ) {
    uint16_t l_returnValue = 0;

    // 2P input
#if 0
    char l_allocatedMemory[ HEAP_MEMORY_SIZE ];

    char* l_remotePlayerInput = &( l_allocatedMemory[ 0 ] );

    recv( g_connectSocket, l_remotePlayerInput, sizeof( uint16_t ), 0 );

    applyInput( INLINE_INPUT(
                    *( reinterpret_cast< uint16_t* >( l_remotePlayerInput ) ) ),
                g_remotePlayer );
#endif

    return ( l_returnValue );
}

extern "C" uint16_t __declspec( dllexport ) gameMode$loading(
    void** _callbackArguments ) {
    uint16_t l_returnValue = 0;

    // 2P input
#if 0
    char l_allocatedMemory[ HEAP_MEMORY_SIZE ];

    char* l_remotePlayerInput = &( l_allocatedMemory[ 0 ] );

    recv( g_connectSocket, l_remotePlayerInput, sizeof( uint16_t ), 0 );

    applyInput( INLINE_INPUT(
                    *( reinterpret_cast< uint16_t* >( l_remotePlayerInput ) ) ),
                g_remotePlayer );
#endif

    return ( l_returnValue );
}

extern "C" uint16_t __declspec( dllexport ) gameMode$inMatch(
    void** _callbackArguments ) {
    uint16_t l_returnValue = 0;

    // 2P input
#if 0
    char l_allocatedMemory[ HEAP_MEMORY_SIZE ];

    char* l_remotePlayerInput = &( l_allocatedMemory[ 0 ] );

    recv( g_connectSocket, l_remotePlayerInput, sizeof( uint16_t ), 0 );

    applyInput( INLINE_INPUT(
                    *( reinterpret_cast< uint16_t* >( l_remotePlayerInput ) ) ),
                g_remotePlayer );
#endif

    return ( l_returnValue );
}

extern "C" uint16_t __declspec( dllexport ) keyboard$getInput$end(
    void** _callbackArguments ) {
    uint16_t l_returnValue = 0;

    std::set< std::string > l_activeMappedKeys = ( direction_t* )_callbackArguments[ 0 ];
    std::set< uint8_t > l_activeKeys = ( button_t* )_callbackArguments[ 1 ];

    {
        if ( g_framesPassed >= 30 ) {
            if ( _activeMappedKeys->find( "ToggleOverlay_KeyConfig_Native" ) !=
                 _activeMappedKeys->end() ) {
                static bool l_wasOverlayToggled = false;
                const bool l_isOverlayToggled =
                    _useCallback( "overlay$Toggle" );

                if ( ( l_isOverlayToggled ) && ( !l_wasOverlayToggled ) ) {
                    l_wasOverlayToggled = true;

                    g_activeFlagsOverlay = SHOW_NATIVE;

                } else if ( ( l_isOverlayToggled ) &&
                            ( l_wasOverlayToggled ) ) {
                    l_wasOverlayToggled = false;

                    g_activeFlagsOverlay &= ~SHOW_NATIVE;
                }

                g_framesPassed = 0;
            }
        }
    }

#if 0

    char l_allocatedMemory[ HEAP_MEMORY_SIZE ];

    char* l_localPlayerInput = &( l_allocatedMemory[ 0 ] );

    *( reinterpret_cast< uint16_t* >( l_localPlayerInput ) ) =
        COMBINE_INPUT( l_buttons, l_direction );

    send( g_connectSocket, l_localPlayerInput, sizeof( uint16_t ), 0 );
#endif

    return ( l_returnValue );
}

// host
// {
// #if defined( CLIENT ) || defined( SERVER )

//             WSAStartup( MAKEWORD( 2, 2 ), &g_wsaData );

//             ZeroMemory( &g_hints, sizeof( g_hints ) );

// #if defined( CLIENT )

//             g_hints.ai_family = AF_UNSPEC;
//             g_hints.ai_socktype = SOCK_STREAM;
//             g_hints.ai_protocol = IPPROTO_TCP;

// #elif defined( SERVER ) // CLIENT

//             g_hints.ai_family = AF_INET;
//             g_hints.ai_socktype = SOCK_STREAM;
//             g_hints.ai_protocol = IPPROTO_TCP;
//             g_hints.ai_flags = AI_PASSIVE;

// #endif // CLIENT

// #if defined( CLIENT )

//             getaddrinfo( "127.0.0.1", "10800", &g_hints, &g_result );

// #elif defined( SERVER ) // CLIENT

//             getaddrinfo( NULL, "10800", &g_hints, &g_result );

// #endif // CLIENT

// #if defined( CLIENT )

//             // Attempt to connect to an address until one succeeds
//             for ( struct addrinfo* _currentAddressInformation = g_result;
//                   _currentAddressInformation != NULL;
//                   _currentAddressInformation =
//                       _currentAddressInformation->ai_next ) {
//                 g_connectSocket =
//                     socket( _currentAddressInformation->ai_family,
//                             _currentAddressInformation->ai_socktype,
//                             _currentAddressInformation->ai_protocol );

//                 if ( g_connectSocket == INVALID_SOCKET ) {
//                     WSACleanup();

//                     return ( 1 );
//                 }

//                 // Connect to server
//                 if ( connect( g_connectSocket,
//                               _currentAddressInformation->ai_addr,
//                               ( int )_currentAddressInformation->ai_addrlen )
//                               ==
//                      SOCKET_ERROR ) {
//                     closesocket( g_connectSocket );

//                     g_connectSocket = INVALID_SOCKET;

//                     continue;
//                 }

//                 break;
//             }

// #elif defined( SERVER ) // CLIENT

//             g_connectSocket =
//                 socket( g_result->ai_family, g_result->ai_socktype,
//                         g_result->ai_protocol );

//             bind( g_connectSocket, g_result->ai_addr,
//                   ( int )g_result->ai_addrlen );

// #endif // CLIENT

//             freeaddrinfo( g_result );

// #if defined( SERVER )

//             listen( g_connectSocket, SOMAXCONN );

//             SOCKET l_buffer = accept( g_connectSocket, NULL, NULL );

//             closesocket( g_connectSocket );

//             g_connectSocket = l_buffer;

// #endif // SERVER

// #endif // CLIENT || SERVER
// }

// un host
// {
// #if defined( CLIENT ) || defined( SERVER )

//             // Cleanup
//             shutdown( g_connectSocket, SD_SEND );
//             closesocket( g_connectSocket );
//             WSACleanup();

// #endif // CLIENT || SERVER
// }

extern "C" uint16_t __declspec( dllexport ) IDirect3D9Ex$CreateDevice(
    void** _callbackArguments ) {
    uint16_t l_returnValue = 0;

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

    g_useCallback =
        ( useCallbackFunction_t )GetProcAddress( l_statesDll, "useCallback" );

    if ( !g_useCallback ) {
        exit( 1 );
    }

    return ( l_returnValue );
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
                    uint16_t l_index = 0;

                    for ( const std::pair<
                              std::string,
                              std::pair< std::vector< std::string >, void* > >&
                              _item : g_overlayLayout ) {
                        // Key
                        { printf( "%s\n", _item.first ); }

                        // Value
                        {
                            const std::pair< std::vector< std::string >,
                                             void* >& _value = _item.second;

                            for ( const std::string& _valueName :
                                  _value.first ) {
                                printf( "%s\n", _valueName );
                            }

                            printf( "%d\n", _value.second );
                        }
                    }

                    //                  if ( 0 ) {
                    //                      const int32_t l_rowY =
                    //                          ( 50 + l_overlayY +
                    //                            ( int32_t )( ( l_index *
                    //                            l_fontSize ) +
                    //                                         ( l_index *
                    //                                           l_rowTopMargin
                    //                                           ) ) );
                    //                      std::string l_valueContent =
                    //                          _item.value().template get<
                    //                          std::string >();

                    //                      const uint32_t l_valueWidth =
                    //                          ( l_valueContent.length() *
                    //                          l_fontSize );

                    //                      std::string l_keyContent = "";

                    //                      const uint32_t l_keyWidth =
                    //                          ( l_keyContent.length() *
                    //                          l_fontSize );
                    //                      const int32_t l_keyX =
                    //                          ( *( int32_t* )SCREEN_WIDTH -
                    //                          l_keyWidth -
                    //                            ( l_leftMargin * 2 ) );

                    //                      l_texts.push_back( { l_layer,
                    //                                           l_alpha,
                    //                                           l_shade,
                    //                                           l_shade2,
                    //                                           { l_leftMargin,
                    //                                           l_rowY }, {
                    //                                           l_fontSize,
                    //                                           l_fontSize },
                    //                                           l_valueContent
                    //                                           } );

                    //                      l_textBackground.size = {
                    //                          ( l_valueWidth +
                    //                            (
                    //                            l_textBackgroundLeftRightPadding
                    //                            * 2 ) ),
                    //                          ( l_fontSize +
                    //                            (
                    //                            l_textBackgroundTopBottomPadding
                    //                            * 2 ) ) };
                    //                      l_textBackground.coordinates = {
                    //                          ( l_leftMargin -
                    //                          l_textBackgroundLeftRightPadding
                    //                          ), ( l_rowY -
                    //                          l_textBackgroundTopBottomPadding
                    //                          ) };

                    //                      if ( l_index ==
                    //                           ( g_menuCursorIndex -
                    //                           l_firstIndexToShow ) ) {
                    //                          struct rectangle
                    //                          l_textBackgroundSelected =
                    //                              l_textBackground;

                    //                          const uint8_t l_alpha = 0xFF;
                    //                          const uint8_t l_red = 0xFF;
                    //                          const uint8_t l_green = 0;
                    //                          const uint8_t l_blue = 0;
                    //                          uint32_t l_color = 0;

                    //                          _useCallback(
                    //                          "native$getColorForRectangle",
                    //                          5,
                    //                                        &l_alpha, &l_red,
                    //                                        &l_green, &l_blue,
                    //                                        &l_color );

                    //                          l_textBackgroundSelected.colorsForRectangle
                    //                          = {
                    //                              l_color, l_color, l_color,
                    //                              l_color };

                    //                          l_rectangles.push_back(
                    //                          l_textBackgroundSelected );

                    //                      } else {
                    //                          l_rectangles.push_back(
                    //                          l_textBackground );
                    //                      }

                    //                      l_texts.push_back( { l_layer,
                    //                                           l_alpha,
                    //                                           l_shade,
                    //                                           l_shade2,
                    //                                           { l_keyX,
                    //                                           l_rowY }, {
                    //                                           l_fontSize,
                    //                                           l_fontSize },
                    //                                           l_keyContent }
                    //                                           );

                    //                      l_textBackground.size = {
                    //                          ( l_keyWidth +
                    //                            (
                    //                            l_textBackgroundLeftRightPadding
                    //                            * 2 ) ),
                    //                          ( l_fontSize +
                    //                            (
                    //                            l_textBackgroundTopBottomPadding
                    //                            * 2 ) ) };
                    //                      l_textBackground.coordinates = {
                    //                          ( l_keyX -
                    //                          l_textBackgroundLeftRightPadding
                    //                          ), ( l_rowY -
                    //                          l_textBackgroundTopBottomPadding
                    //                          ) };

                    //                      if ( l_index ==
                    //                           ( g_menuCursorIndex -
                    //                           l_firstIndexToShow ) ) {
                    //                          struct rectangle
                    //                          l_textBackgroundSelected =
                    //                              l_textBackground;

                    //                          const uint8_t l_alpha = 0xFF;
                    //                          const uint8_t l_red = 0;
                    //                          const uint8_t l_green = 0x59;
                    //                          const uint8_t l_blue = 0xFF;
                    //                          uint32_t l_color = 0;

                    //                          _useCallback(
                    //                          "native$getColorForRectangle",
                    //                          5,
                    //                                        &l_alpha, &l_red,
                    //                                        &l_green, &l_blue,
                    //                                        &l_color );

                    //                          l_textBackgroundSelected.colorsForRectangle
                    //                          = {
                    //                              l_color, l_color, l_color,
                    //                              l_color };

                    //                          l_rectangles.push_back(
                    //                          l_textBackgroundSelected );

                    //                      } else {
                    //                          l_rectangles.push_back(
                    //                          l_textBackground );
                    //                      }

                    //                      l_index++;
                    //                  }
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

    return ( l_returnValue );
}
