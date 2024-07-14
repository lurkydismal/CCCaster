#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <stdbool.h>

#include "log.h"

FILE* g_fileDescriptor;

BOOL WINAPI DllMain( HMODULE _handle, DWORD _reason, LPVOID _ ) {
    bool l_returnValue = true;

    if ( _reason == DLL_PROCESS_ATTACH ) {
        g_fileDescriptor = fopen( LOG_FILE_NAME "." LOG_FILE_EXTENSION, "w" );

        if ( !g_fileDescriptor ) {
            l_returnValue = false;
        }
    }

    return ( l_returnValue );
}
