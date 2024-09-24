#include "Direct3D9EnableMaximizedWindowedModeShim.h"

#include "_useCallback.h"
#include "_useCallback.hpp"

Direct3D9EnableMaximizedWindowedModeShimProc
    m_pDirect3D9EnableMaximizedWindowedModeShim;

int __attribute__( ( stdcall ) ) Direct3D9EnableMaximizedWindowedModeShim( int _enable ) {
#if defined( LOG_EXPORTED_CALLS )

    const char l_message[] =
        "Direct3D9EnableMaximizedWindowedModeShim ( %d )\n";

    _useCallback( "log$transaction$query", l_message );
    _useCallback( "log$transaction$commit" );

#endif // LOG_EXPORTED_CALLS

    if ( !m_pDirect3D9EnableMaximizedWindowedModeShim ) {
        return ( 0 );
    }

    return ( m_pDirect3D9EnableMaximizedWindowedModeShim( _enable ) );
}
