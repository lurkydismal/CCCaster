#include "D3DPERF_QueryRepeatFrame.h"

#include "_useCallback.h"
#include "_useCallback.hpp"

D3DPERF_QueryRepeatFrameProc m_pD3DPERF_QueryRepeatFrame;

int __attribute__( ( stdcall ) ) D3DPERF_QueryRepeatFrame( void ) {
#if defined( LOG_EXPORTED_CALLS )

    const char l_message[] = "D3DPERF_QueryRepeatFrame ()\n";

    _useCallback( "log$transaction$query", l_message );
    _useCallback( "log$transaction$commit" );

#endif // LOG_EXPORTED_CALLS

    if ( !m_pD3DPERF_QueryRepeatFrame ) {
        return ( false );
    }

    return ( m_pD3DPERF_QueryRepeatFrame() );
}

