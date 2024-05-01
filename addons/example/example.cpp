#if defined( _WIN32 )

#include <windows.h>

#else // _WIN32

#error Only Windows is supported

#endif // _WIN32

#include <cstdint>

extern "C" int16_t __declspec( dllexport ) onGameStarted(
    void** _callbackArguments ) {
    MessageBoxA( NULL,
                 "GameStarted", // box text
                 "on",          // box name
                 0 );

    return ( 0 );
}

extern "C" int16_t __declspec( dllexport ) onHosted(
    void** _callbackArguments ) {
    MessageBoxA( NULL,
                 "Hosted", // box text
                 "on",     // box name
                 0 );

    return ( 0 );
}

extern "C" int16_t __declspec( dllexport ) onConnection(
    void** _callbackArguments ) {
    MessageBoxA( NULL,
                 "Connection", // box text
                 "on",         // box name
                 0 );

    return ( 0 );
}
