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
#include <vector>

#include <glaze/glaze.hpp>

#include "api.hpp"
#include "logg.hpp"
#include "timer.hpp"

namespace {

using data_t = struct data {
    size_t size;
    char* value;
};

using wrapperData_t = struct wrapperData {
    bool play{};
    bool dry_run{};
    std::optional< std::vector< std::string > > addon{};

    std::optional< std::string > addons_dir{};
    std::optional< std::string > load_order{};
    bool no_deps{};
    bool force{};
    std::optional< std::vector< std::string > > disable{};

    uint8_t verbose{};
    bool trace{};
    bool dump_patches{};
    bool dump_graph{};
    bool timings{};

    bool safe_mode{};
    bool sandbox{};
    bool no_patches{};
};

constexpr const std::string g_cccasterName = "./main.so";
void* g_cccasterHandle = nullptr;
bool g_timingsEnabled = false;

inline auto isTruthy( const char* _value ) -> bool {
    if ( _value == nullptr ) {
        return ( false );
    }

    std::string l_value = _value;

    for ( char& l_char : l_value ) {
        l_char =
            static_cast< char >( std::tolower( static_cast< unsigned char >( l_char ) ) );
    }

    return ( l_value == "1" ) || ( l_value == "true" ) || ( l_value == "ok" ) ||
           ( l_value == "yes" );
}

inline auto waitForDebuggerIfNeeded() -> void {
    const char* l_waitDebugger = std::getenv( "WRAPPER_WAIT_DEBUGGER" );

    if ( !isTruthy( l_waitDebugger ) ) {
        return;
    }

    logg::info( "WRAPPER_WAIT_DEBUGGER is enabled; waiting for debugger attach" );

    while ( !IsDebuggerPresent() ) {
        Sleep( 100 );
    }

    logg::info( "Debugger attached; continuing startup" );
}

inline auto logEnabledEnvironmentVariables() -> void {
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

inline auto parseWrapperData( const data_t* _data, wrapperData_t& _out ) -> bool {
    if ( ( _data == nullptr ) || ( _data->value == nullptr ) ) {
        logg::error( "Shared data is null" );

        return ( false );
    }

    const std::string_view l_json{ _data->value, _data->size };

    auto l_ec = glz::read_json( _out, l_json );

    if ( l_ec ) {
        logg::error( "Failed to parse wrapper JSON" );

        return ( false );
    }

    return ( true );
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

    const auto l_initFunction =
        std::bit_cast< wrapper::initFunction_t >( dlsym( g_cccasterHandle, "init" ) );

    {
        const char* l_error = dlerror();

        if ( l_error != nullptr ) {
            logg::error( "dlsym failed: {}", l_error );

            dlclose( g_cccasterHandle );

            return ( false );
        }
    }

    HANDLE l_mapping = OpenFileMappingA( FILE_MAP_READ, FALSE, "Local\\MySharedData" );

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
    wrapperData_t l_wrapperData{};

    if ( !parseWrapperData( l_data, l_wrapperData ) ) {
        UnmapViewOfFile( l_view );
        CloseHandle( l_mapping );
        dlclose( g_cccasterHandle );

        return ( false );
    }

    g_timingsEnabled = l_wrapperData.timings;
    l_attachTimer.setEnabled( g_timingsEnabled );

    logg::debug( "SIZE: '{}', VALUE: '{}'", l_data->size,
                 std::string_view( l_data->value, l_data->size ) );

    logg::info( "CALLING INIT()" );

    const bool l_result = l_initFunction( wrapper::makePatch, wrapper::removePatch,
                                          l_data->value, l_data->size );

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
