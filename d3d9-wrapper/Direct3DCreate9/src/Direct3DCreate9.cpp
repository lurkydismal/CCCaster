#include "Direct3DCreate9.h"

#include "_useCallback.h"
#include "_useCallback.hpp"
#include "../../src/IDirect3D9Ex.h"

Direct3DCreate9Proc m_pDirect3DCreate9;

IDirect3D9* __attribute__( ( stdcall ) ) Direct3DCreate9( unsigned int SDKVersion ) {
#if defined( LOG_EXPORTED_CALLS )

    const char l_message[] = "Direct3DCreate9 ( %lu )\n";

    _useCallback( "log$transaction$query", l_message );
    _useCallback( "log$transaction$commit" );

#endif // LOG_EXPORTED_CALLS

    if ( !m_pDirect3DCreate9 ) {
        return ( nullptr );
    }

    IDirect3D9* pD3D9 = m_pDirect3DCreate9( SDKVersion );

    if ( pD3D9 ) {
        return ( new m_IDirect3D9Ex( ( IDirect3D9Ex* )pD3D9, IID_IDirect3D9 ) );
    }

    return ( nullptr );
}
