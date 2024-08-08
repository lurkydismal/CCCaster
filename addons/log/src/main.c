#include <stdbool.h>

#include "log.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#define ATTACH 1

FILE* g_fileDescriptor;

int32_t __attribute( ( stdcall ) ) DllMain( void* _handle,
                                            uint32_t _reason,
                                            void* _ ) {
    bool l_returnValue = true;

    if ( _reason == ATTACH ) {
        AllocConsole();
        freopen( "CONOUT$", "w", stdout );

        g_fileDescriptor = fopen( LOG_FILE_NAME "." LOG_FILE_EXTENSION, "w" );

        if ( !g_fileDescriptor ) {
            l_returnValue = false;
        }
    }

    return ( l_returnValue );
}
