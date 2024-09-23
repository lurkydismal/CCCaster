#include "PSGPErrorProc.h"

#include "_useCallback.h"
#include "_useCallback.hpp"

PSGPErrorProc m_pPSGPError;

long __attribute__( ( stdcall ) ) PSGPError( void ) {
#if defined( LOG_EXPORTED_CALLS )

    const char l_message[] = "PSGPError ()\n";

    _useCallback( "log$transaction$query", l_message );
    _useCallback( "log$transaction$commit" );

#endif // LOG_EXPORTED_CALLS

    if ( !m_pPSGPError ) {
        return ( E_FAIL );
    }

    return ( m_pPSGPError() );
}
