#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <dlfcn.h>

#include <bit>
#include <format>
#include <iostream>
#include <string>
#include <string_view>

#include "api.hpp"
#include "logg.hpp"

namespace {

using data_t = struct data {
    size_t size;
    char value[];
};

constexpr const std::string g_cccasterName = "./main.so";
void* g_cccasterHandle = nullptr;

auto attach() -> bool {
    logg::info( "WRAPPER ATTACHED" );

    g_cccasterHandle = dlopen( g_cccasterName.c_str(), RTLD_NOW );

    if ( g_cccasterHandle ) {
        // Clear any existing error
        dlerror();

        const auto l_initFunction = std::bit_cast< wrapper::initFunction_t >(
            dlsym( g_cccasterHandle, "init" ) );

        // Check dlsym error
        {
            const char* l_error = dlerror();

            if ( l_error != nullptr ) {
                logg::error( "dlsym failed: {}", l_error );
                return ( false );
            }
        }

        // Get shared file value
        {
            HANDLE l_mapping =
                OpenFileMappingA( FILE_MAP_READ, FALSE, "Local\\MySharedData" );

            if ( !l_mapping ) {
                logg::warning( "OpenFileMappingA failed" );
                return ( false );
            }

            LPVOID l_view = MapViewOfFile( l_mapping, FILE_MAP_READ, 0, 0, 0 );
            if ( !l_view ) {
                logg::warning( "MapViewOfFile failed" );
                CloseHandle( l_mapping );
                return ( false );
            }

            const auto l_data = std::bit_cast< data_t* >( l_view );

            logg::debug( "SIZE: '{}', VALUE: '{}'", l_data->size,
                         std::string_view( l_data->value, l_data->size ) );

            logg::info( "CALLING INIT()" );

            const bool l_result =
                l_initFunction( wrapper::makePatch, wrapper::removePatch,
                                l_data->value, l_data->size );

            if ( l_result ) {
                logg::info( "CCCASTER LOADED" );
            } else {
                logg::error( "CCCASTER FAILED TO INIT" );
            }

            UnmapViewOfFile( l_view );
            CloseHandle( l_mapping );

            return ( l_result );
        }

    } else {
        logg::error( "CCCASTER FAILED TO LOAD: {}", dlerror() );
        return ( false );
    }
}

auto detach() -> bool {
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
            break;
        }
    }

    return ( ( TRUE ) );
}
