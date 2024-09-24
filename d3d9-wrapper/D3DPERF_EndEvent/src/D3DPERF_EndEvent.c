#include "D3DPERF_EndEvent.h"

#include "_useCallback.h"
#include "_useCallback.hpp"

D3DPERF_EndEventProc m_pD3DPERF_EndEvent;

int __attribute__( ( stdcall ) ) D3DPERF_EndEvent( void ) {
#if defined( LOG_EXPORTED_CALLS )

    const char l_message[] = "D3DPERF_EndEvent ()\n";

    _useCallback( "log$transaction$query", l_message );
    _useCallback( "log$transaction$commit" );

#endif // LOG_EXPORTED_CALLS

    if ( !m_pD3DPERF_EndEvent ) {
        return ( 0 );
    }

    return ( m_pD3DPERF_EndEvent() );
}
