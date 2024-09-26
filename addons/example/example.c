#if defined( _WIN32 )

#include <wtypes.h>

#include <winuser.h>

#define MESSAGE_BOX( _text )                 \
    MessageBoxA( NULL, _text, /* box text */ \
                 "on",        /* box name */ \
                 0 )

#else // _WIN32

#error Only Windows is supported

#endif // _WIN32

#include <stdint.h>

int16_t __declspec( dllexport ) onGameStarted( void** _callbackArguments ) {
    int16_t l_returnValue = 0;

    l_returnValue = !( MESSAGE_BOX( "GameStarted" ) );

    return ( l_returnValue );
}

int16_t __declspec( dllexport ) onHosted( void** _callbackArguments ) {
    int16_t l_returnValue = 0;

    l_returnValue = !( MESSAGE_BOX( "Hosted" ) );

    return ( l_returnValue );
}

int16_t __declspec( dllexport ) onConnection( void** _callbackArguments ) {
    int16_t l_returnValue = 0;

    l_returnValue = !( MESSAGE_BOX( "Connection" ) );

    return ( l_returnValue );
}
