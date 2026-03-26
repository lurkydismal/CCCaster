#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <dlfcn.h>

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
        return ( false );
    }

    std::string l_value = _value;

    for ( char& l_char : l_value ) {
        l_char = static_cast< char >(
            std::tolower( static_cast< unsigned char >( l_char ) ) );
    }

    return ( l_value == "1" ) || ( l_value == "true" ) || ( l_value == "ok" ) ||
           ( l_value == "yes" );
}

auto waitForDebuggerIfNeeded() -> void {
    const char* l_waitDebugger = std::getenv( "WRAPPER_WAIT_DEBUGGER" );

    if ( !isTruthy( l_waitDebugger ) ) {
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

    if ( l_waitDebugger != nullptr ) {
        logg::debug( "WRAPPER_WAIT_DEBUGGER='{}'", l_waitDebugger );
    }

    if ( l_logPath != nullptr ) {
        logg::debug( "WRAPPER_LOG='{}'", l_logPath );
    }

    if ( l_debug != nullptr ) {
        logg::debug( "WRAPPER_DEBUG='{}'", l_debug );
    }

    if ( l_trace != nullptr ) {
        logg::debug( "WRAPPER_TRACE='{}'", l_trace );
    }
}

namespace json_cfg {

void skipWs( std::string_view _s, size_t& _i ) {
    while ( _i < _s.size() ) {
        auto const l_c = ( unsigned char )_s[ _i ];

        if ( ( l_c != ' ' ) && ( l_c != '\t' ) && ( l_c != '\n' ) &&
             ( l_c != '\r' ) ) {
            return;
        }

        ++_i;
    }
}

auto consume( std::string_view _s, size_t& _i, char _ch ) -> bool {
    skipWs( _s, _i );

    if ( ( _i >= _s.size() ) || ( _s[ _i ] != _ch ) ) {
        return ( false );
    }

    ++_i;

    return ( true );
}

auto parseString( std::string_view _s, size_t& _i, std::string_view& _out )
    -> bool {
    skipWs( _s, _i );

    if ( ( _i >= _s.size() ) || ( _s[ _i ] != '"' ) ) {
        return ( false );
    }

    ++_i;

    size_t const l_begin = _i;

    while ( _i < _s.size() ) {
        char const l_c = _s[ _i ];

        if ( l_c == '"' ) {
            _out = _s.substr( l_begin, ( _i - l_begin ) );

            ++_i;

            return ( true );
        }

        if ( l_c == '\\' ) {
            return ( false );
        }

        ++_i;
    }

    return ( false );
}

auto parseBool( std::string_view _s, size_t& _i, bool& _out ) -> bool {
    skipWs( _s, _i );

    if ( _s.substr( _i, 4 ) == "true" ) {
        _out = true;

        _i += 4;

        return ( true );
    }

    if ( _s.substr( _i, 5 ) == "false" ) {
        _out = false;

        _i += 5;

        return ( true );
    }

    return ( false );
}

auto parseUint8( std::string_view _s, size_t& _i, uint8_t& _out ) -> bool {
    skipWs( _s, _i );

    if ( _i >= _s.size() ) {
        return ( false );
    }

    uint32_t l_value = 0;
    size_t l_start = _i;
    bool l_hasDigit = false;

    while ( _i < _s.size() ) {
        auto const l_c = ( unsigned char )_s[ _i ];

        if ( ( l_c < '0' ) || ( l_c > '9' ) ) {
            break;
        }

        l_hasDigit = true;
        l_value = ( l_value * 10u ) + ( uint32_t )( l_c - '0' );

        if ( l_value > ( uint32_t )std::numeric_limits< uint8_t >::max() ) {
            return ( false );
        }

        ++_i;
    }

    if ( !l_hasDigit )
        return ( false );

    _out = ( uint8_t )l_value;

    return ( _i > l_start );
}

auto parseValueForKey( std::string_view _key,
                       std::string_view _s,
                       size_t& _i,
                       wrapperData_t& _out ) -> bool {
    if ( _key == "verbose" ) {
        return ( parseUint8( _s, _i, _out.verbose ) );

    } else if ( _key == "trace" ) {
        return ( parseBool( _s, _i, _out.trace ) );

    } else if ( _key == "timings" ) {
        return ( parseBool( _s, _i, _out.timings ) );

    } else if ( _key == "no_patches" ) {
        return ( parseBool( _s, _i, _out.no_patches ) );

    } else {
        return ( false );
    }
}

} // namespace json_cfg

auto parseWrapperData( std::string_view _json )
    -> std::optional< wrapperData_t > {
    size_t l_i = 0;
    wrapperData_t l_out{};

    if ( !json_cfg::consume( _json, l_i, '{' ) ) {
        return ( std::nullopt );
    }

    json_cfg::skipWs( _json, l_i );

    if ( json_cfg::consume( _json, l_i, '}' ) ) {
        return ( l_out );
    }

    while ( true ) {
        std::string_view l_key{};

        if ( !json_cfg::parseString( _json, l_i, l_key ) ) {
            return ( std::nullopt );
        }

        if ( !json_cfg::consume( _json, l_i, ':' ) ) {
            return ( std::nullopt );
        }

        if ( !json_cfg::parseValueForKey( l_key, _json, l_i, l_out ) ) {
            return ( std::nullopt );
        }

        json_cfg::skipWs( _json, l_i );

        if ( json_cfg::consume( _json, l_i, '}' ) ) {
            break;
        }

        if ( !json_cfg::consume( _json, l_i, ',' ) ) {
            return ( std::nullopt );
        }
    }

    json_cfg::skipWs( _json, l_i );

    if ( l_i != _json.size() ) {
        return ( std::nullopt );
    }

    return ( l_out );
}

auto parseWrapperData( const data_t* _data ) -> std::optional< wrapperData_t > {
    if ( ( _data == nullptr ) || ( _data->value == nullptr ) ) {
        logg::error( "Shared data is null" );

        return ( std::nullopt );
    }

    const std::string_view l_json{ _data->value, _data->size };

    const auto l_cfg = parseWrapperData( l_json );

    if ( !l_cfg ) {
        return ( std::nullopt );

    } else {
        return ( l_cfg );
    }
}

auto attach() -> bool {
    timer::scoped_t l_attachTimer{ "wrapper::attach", false };

    waitForDebuggerIfNeeded();
    logEnabledEnvironmentVariables();

    logg::info( "WRAPPER ATTACHED" );

    g_cccasterHandle = dlopen( g_cccasterName.c_str(), RTLD_NOW );

    if ( !g_cccasterHandle ) {
        logg::error( "CCCASTER FAILED TO LOAD: {}", dlerror() );

        return ( false );
    }

    dlerror();

    const auto l_initFunction = std::bit_cast< wrapper::initFunction_t >(
        dlsym( g_cccasterHandle, "init" ) );

    {
        const char* l_error = dlerror();

        if ( l_error != nullptr ) {
            logg::error( "dlsym failed: {}", l_error );

            dlclose( g_cccasterHandle );

            return ( false );
        }
    }

    HANDLE l_mapping =
        OpenFileMappingA( FILE_MAP_READ, FALSE, "Local\\MySharedData" );

    if ( !l_mapping ) {
        logg::warning( "OpenFileMappingA failed" );

        dlclose( g_cccasterHandle );

        return ( false );
    }

    LPVOID l_view = MapViewOfFile( l_mapping, FILE_MAP_READ, 0, 0, 0 );

    if ( !l_view ) {
        logg::warning( "MapViewOfFile failed" );

        CloseHandle( l_mapping );
        dlclose( g_cccasterHandle );

        return ( false );
    }

    const auto l_data = std::bit_cast< data_t* >( l_view );
    std::optional< wrapperData_t > l_wrapperData = parseWrapperData( l_data );

    if ( !l_wrapperData ) {
        UnmapViewOfFile( l_view );
        CloseHandle( l_mapping );
        dlclose( g_cccasterHandle );

        return ( false );
    }

    g_timingsEnabled = l_wrapperData.value().timings;
    l_attachTimer.setEnabled( g_timingsEnabled );

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
    }

    UnmapViewOfFile( l_view );
    CloseHandle( l_mapping );

    return ( l_result );
}

auto detach() -> bool {
    timer::scoped_t l_detachTimer{ "wrapper::detach", g_timingsEnabled };

    logg::info( "WRAPPER DETACHED" );

    if ( g_cccasterHandle ) {
        dlclose( g_cccasterHandle );

        g_cccasterHandle = nullptr;
    }

    return ( true );
}

} // namespace

extern "C" auto APIENTRY DllMain( [[maybe_unused]] HMODULE _hModule,
                                  DWORD _ulReasonForCall,
                                  [[maybe_unused]] LPVOID _lpReserved )
    -> BOOL {
    switch ( _ulReasonForCall ) {
        case DLL_PROCESS_ATTACH: {
            return ( attach() );
        }

        case DLL_PROCESS_DETACH: {
            return ( detach() );
        }

        default: {
        }
    }

    return ( ( TRUE ) );
}
