#include "Direct3DCreate9Ex.h"

#include "../../src/IDirect3D9Ex.h"
#include "_useCallback.h"
#include "_useCallback.hpp"

Direct3DCreate9ExProc m_pDirect3DCreate9Ex;

long __attribute__( ( stdcall ) ) Direct3DCreate9Ex( unsigned int SDKVersion,
                                                     IDirect3D9Ex** ppD3D ) {
#if defined( LOG_EXPORTED_CALLS )

    const char l_message[] = "Direct3DCreate9Ex ( %lu, %p )\n";

    _useCallback( "log$transaction$query", l_message );
    _useCallback( "log$transaction$commit" );

#endif // LOG_EXPORTED_CALLS

    if ( !m_pDirect3DCreate9Ex ) {
        return ( E_FAIL );
    }

    long hr = m_pDirect3DCreate9Ex( SDKVersion, ppD3D );

    if ( SUCCEEDED( hr ) && ppD3D ) {
        *ppD3D = new m_IDirect3D9Ex( *ppD3D, IID_IDirect3D9Ex );
    }

    return ( hr );
}
