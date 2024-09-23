#include "PSGPSampleTextureProc.h"

#include "_useCallback.h"
#include "_useCallback.hpp"

PSGPSampleTextureProc m_pPSGPSampleTexture;

long __attribute__( ( stdcall ) ) PSGPSampleTexture( void ) {
#if defined( LOG_EXPORTED_CALLS )

    const char l_message[] = "PSGPSampleTexture ()\n";

    _useCallback( "log$transaction$query", l_message );
    _useCallback( "log$transaction$commit" );

#endif // LOG_EXPORTED_CALLS

    if ( !m_pPSGPSampleTexture ) {
        return ( E_FAIL );
    }

    return ( m_pPSGPSampleTexture() );
}
