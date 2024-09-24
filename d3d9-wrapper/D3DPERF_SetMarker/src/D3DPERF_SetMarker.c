#include "D3DPERF_SetMarker.h"

#include "_useCallback.h"
#include "_useCallback.hpp"

D3DPERF_SetMarkerProc m_pD3DPERF_SetMarker;

void __attribute__( ( stdcall ) ) D3DPERF_SetMarker( D3DCOLOR _color,
                                                     LPCWSTR _eventName ) {
#if defined( LOG_EXPORTED_CALLS )

    const char l_message[] = "D3DPERF_SetMarker ( %d, %s )\n";

    _useCallback( "log$transaction$query", l_message );
    _useCallback( "log$transaction$commit" );

#endif // LOG_EXPORTED_CALLS

    if ( m_pD3DPERF_SetMarker ) {
        return ( m_pD3DPERF_SetMarker( _color, _eventName ) );
    }
}
