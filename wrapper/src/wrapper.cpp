#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <dlfcn.h>

#include <bit>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <optional>
#include <string>
#include <string_view>

#include "api.hpp"
#include "logg.hpp"
#include "timer.hpp"

namespace {

using data_t = struct data {
    size_t size;
    char* value;
};

using wrapperData_t = struct wrapperData {
    uint8_t verbose{};
    bool trace{};
    bool timings{};

    bool no_patches{};
};

constexpr const std::string g_cccasterName = "./main.so";
void* g_cccasterHandle = nullptr;
bool g_timingsEnabled = false;

auto isTruthy( const char* _value ) -> bool {
    if ( _value == nullptr ) {
        logg::trace( "isTruthy: value is null -> false" );
        return ( false );
    }

    std::string l_value = _value;

    logg::trace( "isTruthy: raw='{}'", l_value );

    for ( char& l_char : l_value ) {
        l_char = static_cast< char >(
            std::tolower( static_cast< unsigned char >( l_char ) ) );
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

    logg::info(
        "WRAPPER_WAIT_DEBUGGER is enabled; waiting for debugger attach" );

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

namespace json_cfg {

void skipWs( std::string_view _s, size_t& _i ) {
    const size_t l_begin = _i;

    while ( _i < _s.size() ) {
        auto const l_c = static_cast< unsigned char >( _s[ _i ] );

        if ( ( l_c != ' ' ) && ( l_c != '\t' ) && ( l_c != '\n' ) &&
             ( l_c != '\r' ) ) {
            break;
        }

        ++_i;
    }

    if ( _i != l_begin ) {
        logg::trace( "skipWs: advanced from {} to {}", l_begin, _i );
    }
}

auto consume( std::string_view _s, size_t& _i, char _ch ) -> bool {
    skipWs( _s, _i );

    if ( ( _i >= _s.size() ) || ( _s[ _i ] != _ch ) ) {
        logg::trace( "consume: expected '{}' at {}, failed", _ch, _i );
        return ( false );
    }

    logg::trace( "consume: matched '{}' at {}", _ch, _i );
    ++_i;

    return ( true );
}

auto parseString( std::string_view _s, size_t& _i, std::string_view& _out )
    -> bool {
    skipWs( _s, _i );

    if ( ( _i >= _s.size() ) || ( _s[ _i ] != '"' ) ) {
        logg::trace( "parseString: expected '\"' at {}, failed", _i );
        return ( false );
    }

    const size_t l_quoteBegin = _i;
    ++_i;

    size_t const l_begin = _i;

    while ( _i < _s.size() ) {
        char const l_c = _s[ _i ];

        if ( l_c == '"' ) {
            _out = _s.substr( l_begin, ( _i - l_begin ) );

            logg::trace( "parseString: parsed '{}' from [{}..{})", _out,
                         l_quoteBegin, _i + 1 );

            ++_i;

            return ( true );
        }

        if ( l_c == '\\' ) {
            logg::trace( "parseString: escape sequences are not supported" );
            return ( false );
        }

        ++_i;
    }

    logg::trace( "parseString: unterminated string starting at {}",
                 l_quoteBegin );
    return ( false );
}

auto parseBool( std::string_view _s, size_t& _i, bool& _out ) -> bool {
    skipWs( _s, _i );

    if ( _s.substr( _i, 4 ) == "true" ) {
        _out = true;
        _i += 4;

        logg::trace( "parseBool: parsed true at position {}", ( _i - 4 ) );
        return ( true );
    }

    if ( _s.substr( _i, 5 ) == "false" ) {
        _out = false;
        _i += 5;

        logg::trace( "parseBool: parsed false at position {}", ( _i - 5 ) );
        return ( true );
    }

    logg::trace( "parseBool: failed at position {}", _i );
    return ( false );
}

auto parseUint8( std::string_view _s, size_t& _i, uint8_t& _out ) -> bool {
    skipWs( _s, _i );

    if ( _i >= _s.size() ) {
        logg::trace( "parseUint8: input ended at {}", _i );
        return ( false );
    }

    uint32_t l_value = 0;
    size_t l_start = _i;
    bool l_hasDigit = false;

    while ( _i < _s.size() ) {
        auto const l_c = static_cast< unsigned char >( _s[ _i ] );

        if ( ( l_c < '0' ) || ( l_c > '9' ) ) {
            break;
        }

        l_hasDigit = true;
        l_value = ( l_value * 10u ) + static_cast< uint32_t >( l_c - '0' );

        if ( l_value > static_cast< uint32_t >(
                           std::numeric_limits< uint8_t >::max() ) ) {
            logg::trace( "parseUint8: value overflow at {}", _i );
            return ( false );
        }

        ++_i;
    }

    if ( !l_hasDigit ) {
        logg::trace( "parseUint8: no digits at {}", l_start );
        return ( false );
    }

    _out = static_cast< uint8_t >( l_value );

    logg::trace( "parseUint8: parsed {} from [{}..{})",
                 static_cast< unsigned >( _out ), l_start, _i );

    return ( _i > l_start );
}

auto parseValueForKey( std::string_view _key,
                       std::string_view _s,
                       size_t& _i,
                       wrapperData_t& _out ) -> bool {
    logg::trace( "parseValueForKey: key='{}' at position {}", _key, _i );

    if ( _key == "verbose" ) {
        const bool l_result = parseUint8( _s, _i, _out.verbose );
        logg::trace( "parseValueForKey: verbose -> {}", l_result );
        return ( l_result );

    } else if ( _key == "trace" ) {
        const bool l_result = parseBool( _s, _i, _out.trace );
        logg::trace( "parseValueForKey: trace -> {}", l_result );
        return ( l_result );

    } else if ( _key == "timings" ) {
        const bool l_result = parseBool( _s, _i, _out.timings );
        logg::trace( "parseValueForKey: timings -> {}", l_result );
        return ( l_result );

    } else if ( _key == "no_patches" ) {
        const bool l_result = parseBool( _s, _i, _out.no_patches );
        logg::trace( "parseValueForKey: no_patches -> {}", l_result );
        return ( l_result );

    } else {
        logg::warn( "parseValueForKey: unknown key '{}'", _key );
        return ( false );
    }
}

} // namespace json_cfg

auto parseWrapperData( std::string_view _json )
    -> std::optional< wrapperData_t > {
    logg::debug( "parseWrapperData: parsing {} bytes", _json.size() );

    size_t l_i = 0;
    wrapperData_t l_out{};

    if ( !json_cfg::consume( _json, l_i, '{' ) ) {
        logg::warn( "parseWrapperData: missing opening brace" );
        return ( std::nullopt );
    }

    json_cfg::skipWs( _json, l_i );

    if ( json_cfg::consume( _json, l_i, '}' ) ) {
        logg::debug( "parseWrapperData: empty object" );
        return ( l_out );
    }

    while ( true ) {
        std::string_view l_key{};

        if ( !json_cfg::parseString( _json, l_i, l_key ) ) {
            logg::warn( "parseWrapperData: failed to parse key at {}", l_i );
            return ( std::nullopt );
        }

        if ( !json_cfg::consume( _json, l_i, ':' ) ) {
            logg::warn( "parseWrapperData: missing ':' after key '{}'", l_key );
            return ( std::nullopt );
        }

        if ( !json_cfg::parseValueForKey( l_key, _json, l_i, l_out ) ) {
            logg::warn( "parseWrapperData: failed to parse value for key '{}'",
                        l_key );
            return ( std::nullopt );
        }

        logg::trace( "parseWrapperData: key '{}' parsed successfully", l_key );

        json_cfg::skipWs( _json, l_i );

        if ( json_cfg::consume( _json, l_i, '}' ) ) {
            logg::debug( "parseWrapperData: end of object reached" );
            break;
        }

        if ( !json_cfg::consume( _json, l_i, ',' ) ) {
            logg::warn( "parseWrapperData: missing ',' or '}' at {}", l_i );
            return ( std::nullopt );
        }

        logg::trace( "parseWrapperData: continuing to next key" );
    }

    json_cfg::skipWs( _json, l_i );

    if ( l_i != _json.size() ) {
        logg::warn( "parseWrapperData: trailing data at {}", l_i );
        return ( std::nullopt );
    }

    logg::debug(
        "parseWrapperData: success verbose={}, trace={}, timings={}, "
        "no_patches={}",
        static_cast< unsigned >( l_out.verbose ), l_out.trace, l_out.timings,
        l_out.no_patches );

    return ( l_out );
}

auto parseWrapperData( const data_t* _data ) -> std::optional< wrapperData_t > {
    if ( ( _data == nullptr ) || ( _data->value == nullptr ) ) {
        logg::error( "Shared data is null" );
        return ( std::nullopt );
    }

    logg::debug( "parseWrapperData(shared): size={}", _data->size );

    const std::string_view l_json{ _data->value, _data->size };

    logg::trace( "parseWrapperData(shared): json='{}'", l_json );

    const auto l_cfg = parseWrapperData( l_json );

    if ( !l_cfg ) {
        logg::warn( "parseWrapperData(shared): JSON parse failed" );
        return ( std::nullopt );
    }

    logg::debug( "parseWrapperData(shared): JSON parse succeeded" );
    return ( l_cfg );
}

auto attach() -> bool {
    timer::scoped_t l_attachTimer{ "wrapper::attach", false };

    logg::info( "attach: startup begin" );

    waitForDebuggerIfNeeded();
    logEnabledEnvironmentVariables();

    logg::info( "WRAPPER ATTACHED" );
    logg::debug( "attach: loading '{}'", g_cccasterName );

    g_cccasterHandle = dlopen( g_cccasterName.c_str(), RTLD_NOW );

    if ( !g_cccasterHandle ) {
        logg::error( "CCCASTER FAILED TO LOAD: {}", dlerror() );
        return ( false );
    }

    logg::debug( "attach: library loaded at handle {}", g_cccasterHandle );

    dlerror();

    const auto l_initFunction = std::bit_cast< wrapper::initFunction_t >(
        dlsym( g_cccasterHandle, "init" ) );

    {
        const char* l_error = dlerror();

        if ( l_error != nullptr ) {
            logg::error( "dlsym failed: {}", l_error );

            dlclose( g_cccasterHandle );
            g_cccasterHandle = nullptr;

            return ( false );
        }
    }

    logg::debug( "attach: init symbol resolved" );

    HANDLE l_mapping =
        OpenFileMappingA( FILE_MAP_READ, FALSE, "Local\\MySharedData" );

    if ( !l_mapping ) {
        logg::warn( "OpenFileMappingA failed: {}", GetLastError() );

        dlclose( g_cccasterHandle );
        g_cccasterHandle = nullptr;

        return ( false );
    }

    logg::debug( "attach: shared mapping opened" );

    LPVOID l_view = MapViewOfFile( l_mapping, FILE_MAP_READ, 0, 0, 0 );

    if ( !l_view ) {
        logg::warn( "MapViewOfFile failed: {}", GetLastError() );

        CloseHandle( l_mapping );
        dlclose( g_cccasterHandle );
        g_cccasterHandle = nullptr;

        return ( false );
    }

    logg::debug( "attach: shared view mapped at {}", l_view );

    const auto l_data = std::bit_cast< data_t* >( l_view );

    if ( l_data == nullptr ) {
        logg::error( "attach: shared data pointer is null" );

        UnmapViewOfFile( l_view );
        CloseHandle( l_mapping );
        dlclose( g_cccasterHandle );
        g_cccasterHandle = nullptr;

        return ( false );
    }

    logg::trace( "attach: shared data struct at {}", l_data );
    logg::trace( "attach: shared data size={}", l_data->size );

    std::optional< wrapperData_t > l_wrapperData = parseWrapperData( l_data );

    if ( !l_wrapperData ) {
        logg::error( "attach: wrapper configuration parse failed" );

        UnmapViewOfFile( l_view );
        CloseHandle( l_mapping );
        dlclose( g_cccasterHandle );
        g_cccasterHandle = nullptr;

        return ( false );
    }

    g_timingsEnabled = l_wrapperData.value().timings;
    l_attachTimer.setEnabled( g_timingsEnabled );

    logg::debug( "attach: timings enabled={}", g_timingsEnabled );
    logg::debug(
        "attach: config verbose={}, trace={}, timings={}, no_patches={}",
        static_cast< unsigned >( l_wrapperData->verbose ), l_wrapperData->trace,
        l_wrapperData->timings, l_wrapperData->no_patches );

    logg::debug( "SIZE: '{}', VALUE: '{}'", l_data->size,
                 std::string_view( l_data->value, l_data->size ) );

    logg::info( "CALLING INIT()" );

    const bool l_result = l_initFunction(
        wrapper::makePatch, wrapper::removePatch, l_data->value, l_data->size );

    if ( l_result ) {
        logg::info( "CCCASTER LOADED" );

    } else {
        logg::error( "CCCASTER FAILED TO INIT" );

        dlclose( g_cccasterHandle );
        g_cccasterHandle = nullptr;
    }

    logg::debug( "attach: unmapping shared view" );
    UnmapViewOfFile( l_view );

    logg::debug( "attach: closing shared mapping handle" );
    CloseHandle( l_mapping );

    logg::info( "attach: completed with result={}", l_result );

    return ( l_result );
}

auto detach() -> bool {
    timer::scoped_t l_detachTimer{ "wrapper::detach", g_timingsEnabled };

    logg::info( "detach: startup" );
    logg::info( "WRAPPER DETACHED" );

    if ( g_cccasterHandle ) {
        logg::debug( "detach: closing library handle {}", g_cccasterHandle );
        dlclose( g_cccasterHandle );
        g_cccasterHandle = nullptr;
        logg::debug( "detach: library handle cleared" );
    } else {
        logg::trace( "detach: no library handle to close" );
    }

    logg::info( "detach: done" );
    return ( true );
}

} // namespace

extern "C" auto APIENTRY DllMain( [[maybe_unused]] HMODULE _hModule,
                                  DWORD _ulReasonForCall,
                                  [[maybe_unused]] LPVOID _lpReserved )
    -> BOOL {
    logg::trace( "DllMain: reason={}", _ulReasonForCall );

    switch ( _ulReasonForCall ) {
        case DLL_PROCESS_ATTACH: {
            logg::debug( "DllMain: DLL_PROCESS_ATTACH" );
            const bool l_result = attach();
            logg::debug( "DllMain: attach returned {}", l_result );
            return ( l_result );
        }

        case DLL_PROCESS_DETACH: {
            logg::debug( "DllMain: DLL_PROCESS_DETACH" );
            const bool l_result = detach();
            logg::debug( "DllMain: detach returned {}", l_result );
            return ( l_result );
        }

        default: {
            logg::trace( "DllMain: unhandled reason {}", _ulReasonForCall );
        }
    }

    return ( TRUE );
}
