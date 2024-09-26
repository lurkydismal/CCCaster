#include <wtypes.h>

#include <stdbool.h>
#include <wincon.h>

#include "log.h"

#define ATTACH 1

FILE* g_fileDescriptor;

int32_t __attribute( ( stdcall ) ) DllMain( void* _handle,
                                            uint32_t _reason,
                                            void* _ ) {
    bool l_returnValue = true;

    if ( _reason == ATTACH ) {
#if defined( PRINT_LOG )

        AllocConsole();
        freopen( "CONOUT$", "w", stdout );

#endif

        g_fileDescriptor = fopen( LOG_FILE_NAME "." LOG_FILE_EXTENSION, "w" );

        if ( !g_fileDescriptor ) {
            l_returnValue = false;
        }
    }

    return ( l_returnValue );
}
