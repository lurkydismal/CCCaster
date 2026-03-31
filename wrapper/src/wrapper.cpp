#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <dlfcn.h>

#include <bit>
#include <cstdlib>
#include <optional>
#include <string>

#include "api.hpp"
#include "logg.hpp"
#include "timer.hpp"
#include "wrapper/config_parser.hpp"
#include "wrapper/runtime_env.hpp"

namespace {

const std::string g_cccasterName = "./main.so";
void* g_cccasterHandle = nullptr;
bool g_timingsEnabled = false;
const wrapper::apiVtable_t g_apiVtable{
    .makePatch = wrapper::makePatch,
    .removePatch = wrapper::removePatch,
    .readMemory = wrapper::readMemory,
    .writeMemory = wrapper::writeMemory,
};

auto resetLibraryHandle() -> void {
    if ( g_cccasterHandle != nullptr ) {
        dlclose( g_cccasterHandle );
        g_cccasterHandle = nullptr;
    }
}

auto mapSharedData( HANDLE& _mapping, LPVOID& _view ) -> bool {
    _mapping = OpenFileMappingA( FILE_MAP_READ, FALSE, "Local\\MySharedData" );
    if ( !_mapping ) {
        logg::warning( "OpenFileMappingA failed: {}", GetLastError() );
        return ( false );
    }

    _view = MapViewOfFile( _mapping, FILE_MAP_READ, 0, 0, 0 );
    if ( !_view ) {
        logg::warning( "MapViewOfFile failed: {}", GetLastError() );
        CloseHandle( _mapping );
        _mapping = nullptr;
        return ( false );
    }

    return ( true );
}

auto unmapSharedData( HANDLE _mapping, LPVOID _view ) -> void {
    if ( _view != nullptr ) {
        UnmapViewOfFile( _view );
    }
    if ( _mapping != nullptr ) {
        CloseHandle( _mapping );
    }
}

auto configureFromSharedData( const wrappercfg::data_t* _data, timer::scoped_t& _attachTimer )
    -> std::optional< wrappercfg::wrapperData_t > {
    const auto l_wrapperData = wrappercfg::parseWrapperData( _data );
    if ( !l_wrapperData ) {
        logg::error( "attach: wrapper configuration parse failed" );
        return ( std::nullopt );
    }

    wrapper::setNoPatches( l_wrapperData->no_patches );
    g_timingsEnabled = l_wrapperData->timings;
    _attachTimer.setEnabled( g_timingsEnabled );
    return ( l_wrapperData );
}

auto callInit( wrapper::initFunction_t _initFunction, const wrappercfg::data_t* _data ) -> bool {
    const std::string l_json{ _data->value, _data->size };
    return ( _initFunction( &g_apiVtable, l_json.c_str(), _data->size ) );
}

auto attach() -> bool {
    timer::scoped_t l_attachTimer{ "wrapper::attach", false };

    logg::info( "attach: startup begin" );
    wrapperruntime::waitForDebuggerIfNeeded();
    wrapperruntime::logEnabledEnvironmentVariables();

    g_cccasterHandle = dlopen( g_cccasterName.c_str(), RTLD_NOW );
    if ( !g_cccasterHandle ) {
        logg::error( "CCCASTER FAILED TO LOAD: {}", dlerror() );
        return ( false );
    }

    const auto l_initFunction = std::bit_cast< wrapper::initFunction_t >( dlsym( g_cccasterHandle, "init" ) );
    if ( const char* l_error = dlerror(); l_error != nullptr ) {
        logg::error( "dlsym failed: {}", l_error );
        resetLibraryHandle();
        return ( false );
    }

    HANDLE l_mapping = nullptr;
    LPVOID l_view = nullptr;
    if ( !mapSharedData( l_mapping, l_view ) ) {
        resetLibraryHandle();
        return ( false );
    }

    const auto l_data = std::bit_cast< wrappercfg::data_t* >( l_view );
    if ( l_data == nullptr ) {
        logg::error( "attach: shared data pointer is null" );
        unmapSharedData( l_mapping, l_view );
        resetLibraryHandle();
        return ( false );
    }

    if ( !configureFromSharedData( l_data, l_attachTimer ) ) {
        unmapSharedData( l_mapping, l_view );
        resetLibraryHandle();
        return ( false );
    }

    const bool l_result = callInit( l_initFunction, l_data );
    if ( !l_result ) {
        logg::error( "CCCASTER FAILED TO INIT" );
        resetLibraryHandle();
    }

    unmapSharedData( l_mapping, l_view );
    return ( l_result );
}

auto detach() -> bool {
    timer::scoped_t l_detachTimer{ "wrapper::detach", g_timingsEnabled };
    (void)l_detachTimer;

    resetLibraryHandle();
    return ( true );
}

} // namespace

extern "C" auto APIENTRY DllMain( [[maybe_unused]] HMODULE _hModule,
                                  DWORD _ulReasonForCall,
                                  [[maybe_unused]] LPVOID _lpReserved )
    -> BOOL {
    {
        const char* l_trace = std::getenv( "WRAPPER_TRACE" );

        if ( wrapperruntime::isTruthy( l_trace ) ) {
            logg::setLogLevel( logg::level_t::trace );

        } else {
            const char* l_debug = std::getenv( "WRAPPER_DEBUG" );

            if ( wrapperruntime::isTruthy( l_debug ) ) {
                logg::setLogLevel( logg::level_t::debug );
            }
        }
    }

    switch ( _ulReasonForCall ) {
        case DLL_PROCESS_ATTACH:
            return ( attach() );

        case DLL_PROCESS_DETACH:
            return ( detach() );

        default:
            break;
    }

    return ( TRUE );
}
