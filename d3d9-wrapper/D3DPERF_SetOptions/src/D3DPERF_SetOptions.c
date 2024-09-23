#include "D3DPERF_SetOptions.h"

#include "_useCallback.h"
#include "_useCallback.hpp"

D3DPERF_SetOptionsProc m_pD3DPERF_SetOptions;

void __attribute__( ( stdcall ) ) D3DPERF_SetOptions( unsigned long _options ) {
#if defined( LOG_EXPORTED_CALLS )

    const char l_message[] = "D3DPERF_SetOptions ( %d )\n";

    _useCallback( "log$transaction$query", l_message );
    _useCallback( "log$transaction$commit" );

#endif // LOG_EXPORTED_CALLS

    if ( m_pD3DPERF_SetOptions ) {
        return ( m_pD3DPERF_SetOptions( _options ) );
    }
}
