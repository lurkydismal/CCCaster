#include "Direct3DShaderValidatorCreate9.h"

#include "_useCallback.h"
#include "_useCallback.hpp"

Direct3DShaderValidatorCreate9Proc m_pDirect3DShaderValidatorCreate9;

long __declspec( dllexport ) __attribute__( ( stdcall ) )
Direct3DShaderValidatorCreate9( void ) {
#if defined( LOG_EXPORTED_CALLS )

    const char l_message[] = "Direct3DShaderValidatorCreate9 ()\n";

    _useCallback( "log$transaction$query", l_message );
    _useCallback( "log$transaction$commit" );

#endif // LOG_EXPORTED_CALLS

    if ( !m_pDirect3DShaderValidatorCreate9 ) {
        return ( E_FAIL );
    }

    return ( m_pDirect3DShaderValidatorCreate9() );
}
