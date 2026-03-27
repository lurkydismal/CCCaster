#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <dlfcn.h>

#include <algorithm>
#include <bit>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <optional>
#include <string>
#include <string_view>

#include "api.hpp"
#include "logg.hpp"
#include "timer.hpp"

namespace {

using data_t = struct data {
    size_t size;
    char value[];
};

} // namespace

template <>
struct std::formatter< data_t > {
    constexpr auto parse( std::format_parse_context& _ctx ) {
        return _ctx.begin();
    }

    auto format( const data_t& _data, std::format_context& _ctx ) const {
        if ( _data.value == nullptr ) {
            return std::format_to(
                _ctx.out(), "data_t{{ size={}, value=null }}", _data.size );
        }

        std::string_view l_view{ _data.value, _data.size };

        return std::format_to( _ctx.out(), "data_t{{ size={}, value='{}' }}",
                               _data.size, l_view );
    }
};

namespace {

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

auto trim( std::string_view _s ) -> std::string_view {
    while ( !_s.empty() &&
            std::isspace( static_cast< unsigned char >( _s.front() ) ) ) {
        _s.remove_prefix( 1 );
    }

    while ( !_s.empty() &&
            std::isspace( static_cast< unsigned char >( _s.back() ) ) ) {
        _s.remove_suffix( 1 );
    }

    return ( _s );
}

auto lowerCopy( std::string_view _s ) -> std::string {
    std::string l_out{ _s };

    std::ranges::transform(
        l_out, l_out.begin(), []( unsigned char _ch ) -> char {
            return ( static_cast< char >( std::tolower( _ch ) ) );
        } );

    return ( l_out );
}

auto parseBoolEnv( char const* _name,
                   bool _default = false,
                   bool* _ok = nullptr ) -> bool {
    if ( _ok != nullptr ) {
        *_ok = true;
    }

    char const* l_raw = std::getenv( _name );
    if ( l_raw == nullptr ) {
        if ( _ok != nullptr ) {
            *_ok = false;
        }
        return ( _default );
    }

    std::string_view l_value{ l_raw };
    l_value = trim( l_value );
    std::string l_lower = lowerCopy( l_value );

    if ( l_lower == "1" || l_lower == "true" || l_lower == "yes" ||
         l_lower == "ok" || l_lower == "on" ) {
        return ( true );
    }

    if ( l_lower == "0" || l_lower == "false" || l_lower == "no" ||
         l_lower == "off" ) {
        return ( false );
    }

    if ( _ok != nullptr ) {
        *_ok = false;
    }

    return ( _default );
}

auto parseVerboseEnv( char const* _name,
                      uint8_t _default = 0,
                      bool* _ok = nullptr ) -> uint8_t {
    if ( _ok != nullptr ) {
        *_ok = true;
    }

    char const* l_raw = std::getenv( _name );
    if ( l_raw == nullptr ) {
        if ( _ok != nullptr ) {
            *_ok = false;
        }
        return ( _default );
    }

    std::string_view l_value{ l_raw };
    l_value = trim( l_value );

    if ( l_value.empty() ) {
        if ( _ok != nullptr ) {
            *_ok = false;
        }
        return ( _default );
    }

    for ( char const l_ch : l_value ) {
        if ( !std::isdigit( static_cast< unsigned char >( l_ch ) ) ) {
            if ( _ok != nullptr ) {
                *_ok = false;
            }
            return ( _default );
        }
    }

    unsigned long l_num = 0;

    auto const* l_begin = l_value.begin();
    auto const* l_end = l_value.end();

    std::from_chars_result l_res = std::from_chars( l_begin, l_end, l_num, 10 );

    if ( l_res.ec != std::errc{} || l_res.ptr != l_end ) {
        if ( _ok != nullptr ) {
            *_ok = false;
        }
        return ( _default );
    }

    if ( l_num > 3 ) {
        if ( _ok != nullptr ) {
            *_ok = false;
        }
        return ( 3 );
    }

    return ( static_cast< uint8_t >( l_num ) );
}

auto parseWrapperData() -> std::optional< wrapperData_t > {
    wrapperData_t l_out{};

    bool l_ok = false;

    l_out.verbose = parseVerboseEnv( "WRAPPER_VERBOSE", 0, &l_ok );

    if ( !l_ok && std::getenv( "WRAPPER_VERBOSE" ) != nullptr ) {
        logg::warning( "parseWrapperData: invalid WRAPPER_VERBOSE" );

        return ( std::nullopt );
    }

    l_out.trace = parseBoolEnv( "WRAPPER_TRACE", false, &l_ok );

    if ( !l_ok && std::getenv( "WRAPPER_TRACE" ) != nullptr ) {
        logg::warning( "parseWrapperData: invalid WRAPPER_TRACE" );

        return ( std::nullopt );
    }

    l_out.timings = parseBoolEnv( "WRAPPER_TIMINGS", false, &l_ok );

    if ( !l_ok && std::getenv( "WRAPPER_TIMINGS" ) != nullptr ) {
        logg::warning( "parseWrapperData: invalid WRAPPER_TIMINGS" );

        return ( std::nullopt );
    }

    l_out.no_patches = parseBoolEnv( "WRAPPER_NO_PATCHES", false, &l_ok );

    if ( !l_ok && std::getenv( "WRAPPER_NO_PATCHES" ) != nullptr ) {
        logg::warning( "parseWrapperData: invalid WRAPPER_NO_PATCHES" );

        return ( std::nullopt );
    }

    if ( parseBoolEnv( "WRAPPER_DEBUG", false ) ) {
        l_out.verbose = std::max< uint8_t >( l_out.verbose, 1 );
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

    const auto l_cfg = parseWrapperData();

    if ( !l_cfg ) {
        logg::warning( "parseWrapperData(env): JSON parse failed" );
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
        logg::warning( "OpenFileMappingA failed: {}", GetLastError() );

        dlclose( g_cccasterHandle );
        g_cccasterHandle = nullptr;

        return ( false );
    }

    logg::debug( "attach: shared mapping opened" );

    LPVOID l_view = MapViewOfFile( l_mapping, FILE_MAP_READ, 0, 0, 0 );

    if ( !l_view ) {
        logg::warning( "MapViewOfFile failed: {}", GetLastError() );

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

    logg::trace( "attach: shared data struct size={}", l_data->size );
    logg::trace( "attach: shared data struct value='{}'",
                 std::string_view( l_data->value, l_data->size ) );

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

    logg::info( "CALLING INIT()" );

    const std::string l_value = l_data->value;

    logg::trace( "attach: init json value='{}'", l_value.c_str() );

    const bool l_result =
        l_initFunction( wrapper::makePatch, wrapper::removePatch,
                        l_value.c_str(), l_data->size );

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
