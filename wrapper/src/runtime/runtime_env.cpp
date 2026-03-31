#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <cctype>
#include <cstdlib>
#include <string>

#include "logg.hpp"
#include "wrapper/runtime_env.hpp"

namespace wrapperruntime {

auto isTruthy( const char* _value ) -> bool {
    if ( _value == nullptr ) {
        logg::trace( "isTruthy: value is null -> false" );
        return ( false );
    }

    std::string l_value = _value;

    logg::trace( "isTruthy: raw='{}'", l_value );

    for ( char& l_char : l_value ) {
        l_char = static_cast< char >( std::tolower( static_cast< unsigned char >( l_char ) ) );
    }

    const bool l_result = ( l_value == "1" ) || ( l_value == "true" ) ||
                          ( l_value == "ok" ) || ( l_value == "yes" );

    logg::trace( "isTruthy: normalized='{}' -> {}", l_value, l_result );

    return ( l_result );
}

auto waitForDebuggerIfNeeded() -> void {
    const char* l_waitDebugger = std::getenv( "WRAPPER_WAIT_DEBUGGER" );

    logg::debug( "Checking WRAPPER_WAIT_DEBUGGER" );

    if ( !isTruthy( l_waitDebugger ) ) {
        logg::trace( "Debugger wait disabled" );
        return;
    }

    logg::info( "WRAPPER_WAIT_DEBUGGER is enabled; waiting for debugger attach" );

    while ( !IsDebuggerPresent() ) {
        Sleep( 100 );
    }

    logg::info( "Debugger attached; continuing startup" );
}

auto logEnabledEnvironmentVariables() -> void {
    const char* l_waitDebugger = std::getenv( "WRAPPER_WAIT_DEBUGGER" );
    const char* l_logPath = std::getenv( "WRAPPER_LOG" );
    const char* l_debug = std::getenv( "WRAPPER_DEBUG" );
    const char* l_trace = std::getenv( "WRAPPER_TRACE" );

    logg::debug( "Reading wrapper environment variables" );

    if ( l_waitDebugger != nullptr ) {
        logg::debug( "WRAPPER_WAIT_DEBUGGER='{}'", l_waitDebugger );
    } else {
        logg::trace( "WRAPPER_WAIT_DEBUGGER is not set" );
    }

    if ( l_logPath != nullptr ) {
        logg::debug( "WRAPPER_LOG='{}'", l_logPath );
    } else {
        logg::trace( "WRAPPER_LOG is not set" );
    }

    if ( l_debug != nullptr ) {
        logg::debug( "WRAPPER_DEBUG='{}'", l_debug );
    } else {
        logg::trace( "WRAPPER_DEBUG is not set" );
    }

    if ( l_trace != nullptr ) {
        logg::debug( "WRAPPER_TRACE='{}'", l_trace );
    } else {
        logg::trace( "WRAPPER_TRACE is not set" );
    }
}

} // namespace wrapperruntime
