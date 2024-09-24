#include "D3DPERF_GetStatus.h"

#include "_useCallback.h"
#include "_useCallback.hpp"

D3DPERF_GetStatusProc m_pD3DPERF_GetStatus;

unsigned long __attribute__( ( stdcall ) ) D3DPERF_GetStatus( void ) {
#if defined( LOG_EXPORTED_CALLS )

    const char l_message[] = "D3DPERF_GetStatus ()\n";

    _useCallback( "log$transaction$query", l_message );
    _useCallback( "log$transaction$commit" );

#endif // LOG_EXPORTED_CALLS

    if ( !m_pD3DPERF_GetStatus ) {
        return ( 0 );
    }

    return ( m_pD3DPERF_GetStatus() );
}
