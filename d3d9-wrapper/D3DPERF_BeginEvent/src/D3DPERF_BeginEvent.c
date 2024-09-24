#include "D3DPERF_BeginEvent.h"

#include "_useCallback.h"
#include "_useCallback.hpp"

D3DPERF_BeginEventProc m_pD3DPERF_BeginEvent;

int __attribute__( ( stdcall ) ) D3DPERF_BeginEvent(
    D3DCOLOR _color,
    LPCWSTR _eventName ) {
#if defined( LOG_EXPORTED_CALLS )

    const char l_message[] = "D3DPERF_BeginEvent ( %d, %s )\n";

    _useCallback( "log$transaction$query", l_message );
    _useCallback( "log$transaction$commit" );

#endif // LOG_EXPORTED_CALLS

    if ( !m_pD3DPERF_BeginEvent ) {
        return ( 0 );
    }

    return ( m_pD3DPERF_BeginEvent( _color, _eventName ) );
}
