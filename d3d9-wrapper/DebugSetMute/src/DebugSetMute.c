#include "DebugSetMute.h"

#include "_useCallback.h"
#include "_useCallback.hpp"

DebugSetMuteProc m_pDebugSetMute;

void __attribute__( ( stdcall ) ) DebugSetMute( void ) {
#if defined( LOG_EXPORTED_CALLS )

    const char l_message[] = "DebugSetMute ()\n";

    _useCallback( "log$transaction$query", l_message );
    _useCallback( "log$transaction$commit" );

#endif // LOG_EXPORTED_CALLS

    if ( m_pDebugSetMute ) {
        return ( m_pDebugSetMute() );
    }
}
