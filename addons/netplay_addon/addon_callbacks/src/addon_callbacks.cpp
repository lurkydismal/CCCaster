#include "addon_callbacks.hpp"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <winsock2.h>
#include <ws2tcpip.h>

#include <vector>

#include "_useCallback.hpp"
#include "applyInput.hpp"
#include "imgui.hpp"

#pragma comment( lib, "user32.lib" )
#pragma comment( lib, "ws2_32.lib" )
#pragma comment( lib, "mswsock.lib" )

#define HEAP_MEMORY_SIZE 150

useCallbackFunction_t g_useCallback = NULL;

namespace {

#if defined( CLIENT ) || defined( SERVER )

WSADATA g_wsaData;
struct addrinfo g_hints;
struct addrinfo* g_result;
SOCKET g_connectSocket;

#endif // CLIENT || SERVER

#if defined( CLIENT )

const player_t g_remotePlayer = FIRST;

#elif defined( SERVER )

const player_t g_remotePlayer = SECOND;

#endif // CLIENT

uint32_t g_activeFlagsNetplay = 0;

} // namespace

extern "C" uint16_t __declspec( dllexport )
    gameMode$opening( void** _callbackArguments ) {
    if ( !g_useCallback ) {
        HMODULE l_statesDll = GetModuleHandleA( "states.dll" );

        if ( !l_statesDll ) {
            exit( 1 );
        }

        g_useCallback = ( useCallbackFunction_t )GetProcAddress(
            l_statesDll, "useCallback" );

        if ( !g_useCallback ) {
            exit( 1 );
        }
    }

    return ( 0 );
}

extern "C" uint16_t __declspec( dllexport )
    gameMode$characterSelect( void** _callbackArguments ) {
    // 2P input
#if defined( CLIENT ) || defined( SERVER )

    char l_allocatedMemory[ HEAP_MEMORY_SIZE ];

    char* l_remotePlayerInput = &( l_allocatedMemory[ 0 ] );

    recv( g_connectSocket, l_remotePlayerInput, sizeof( uint16_t ), 0 );

    applyInput( INLINE_INPUT(
                    *( reinterpret_cast< uint16_t* >( l_remotePlayerInput ) ) ),
                g_remotePlayer );

#endif // CLIENT || SERVER

    return ( 0 );
}

extern "C" uint16_t __declspec( dllexport )
    gameMode$loading( void** _callbackArguments ) {
    // 2P input
#if defined( CLIENT ) || defined( SERVER )

    char l_allocatedMemory[ HEAP_MEMORY_SIZE ];

    char* l_remotePlayerInput = &( l_allocatedMemory[ 0 ] );

    recv( g_connectSocket, l_remotePlayerInput, sizeof( uint16_t ), 0 );

    applyInput( INLINE_INPUT(
                    *( reinterpret_cast< uint16_t* >( l_remotePlayerInput ) ) ),
                g_remotePlayer );

#endif // CLIENT || SERVER

    return ( 0 );
}

extern "C" uint16_t __declspec( dllexport )
    gameMode$inMatch( void** _callbackArguments ) {
    // 2P input
#if defined( CLIENT ) || defined( SERVER )

    char l_allocatedMemory[ HEAP_MEMORY_SIZE ];

    char* l_remotePlayerInput = &( l_allocatedMemory[ 0 ] );

    recv( g_connectSocket, l_remotePlayerInput, sizeof( uint16_t ), 0 );

    applyInput( INLINE_INPUT(
                    *( reinterpret_cast< uint16_t* >( l_remotePlayerInput ) ) ),
                g_remotePlayer );

#endif // CLIENT || SERVER

    return ( 0 );
}

extern "C" uint16_t __declspec( dllexport )
    keyboard$getInput$end( void** _callbackArguments ) {
#if defined( CLIENT ) || defined( SERVER )
    direction_t* l_direction = ( direction_t* )_callbackArguments[ 0 ];
    button_t* l_buttons = ( button_t* )_callbackArguments[ 1 ];

    char l_allocatedMemory[ HEAP_MEMORY_SIZE ];

    char* l_localPlayerInput = &( l_allocatedMemory[ 0 ] );

    *( reinterpret_cast< uint16_t* >( l_localPlayerInput ) ) =
        COMBINE_INPUT( l_buttons, l_direction );

    send( g_connectSocket, l_localPlayerInput, sizeof( uint16_t ), 0 );

#endif // CLIENT || SERVER

    return ( 0 );
}

extern "C" uint16_t __declspec( dllexport )
    overlay$beforeDraw$ImGui( void** _callbackArguments ) {
    if ( g_activeFlagsNetplay & SHOW_OVERLAY_NETPLAY ) {
        ImGui_Text_t l_Text =
            reinterpret_cast< ImGui_Text_t >( _callbackArguments[ 0 ] );

        l_Text( "Hello from beforeDraw$ImGui!" );

        const char* l_inputTextPlaceholder = "test";
        static std::string l_test = "a";

        _useCallback( "g_ImGui$InputText", 2, &l_inputTextPlaceholder,
                      &l_test );

        const char* l_buttonLabel = l_test.c_str();
        const ImVec2 l_buttonSize = ImVec2( 0, 0 );

        uint16_t l_result =
            _useCallback( "ImGui$Button", 2, &l_buttonLabel, &l_buttonSize );

        if ( ( l_result ) && ( l_result != ENODATA ) ) {
            MessageBoxA( 0, "g_ImGui$Button", "t", 0 );
        }
    }

    return ( 0 );
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
