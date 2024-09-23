#include "D3DPERF_SetRegion.h"

#include "_useCallback.h"
#include "_useCallback.hpp"

D3DPERF_SetRegionProc m_pD3DPERF_SetRegion;

void __attribute__( ( stdcall ) ) D3DPERF_SetRegion( D3DCOLOR _color, LPCWSTR _eventName ) {
#if defined( LOG_EXPORTED_CALLS )

    const char l_message[] = "D3DPERF_SetRegion ( %d, %s )\n";

    _useCallback( "log$transaction$query", l_message );
    _useCallback( "log$transaction$commit" );

#endif // LOG_EXPORTED_CALLS

    if ( m_pD3DPERF_SetRegion ) {
        return ( m_pD3DPERF_SetRegion( _color, _eventName ) );
    }
}
