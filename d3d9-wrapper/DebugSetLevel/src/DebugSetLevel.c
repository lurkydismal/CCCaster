#include "DebugSetLevel.h"

#include "_useCallback.h"
#include "_useCallback.hpp"

DebugSetLevelProc m_pDebugSetLevel;

long __attribute__( ( stdcall ) ) DebugSetLevel( unsigned long _level ) {
#if defined( LOG_EXPORTED_CALLS )

    const char l_message[] = "DebugSetLevel ( %d )\n";

    _useCallback( "log$transaction$query", l_message );
    _useCallback( "log$transaction$commit" );

#endif // LOG_EXPORTED_CALLS

    if ( !m_pDebugSetLevel ) {
        return ( E_FAIL );
    }

    return ( m_pDebugSetLevel( _level ) );
}
