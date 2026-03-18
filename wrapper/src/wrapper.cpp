#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <dlfcn.h>

#include <iostream>
#include <string>

#include "api.hpp"

namespace {

constexpr const std::string g_cccasterName = "./main.so";
void* g_cccasterHandle = nullptr;

auto attach() -> bool {
    std::cout << "WRAPPER ATTACHED\n";

    g_cccasterHandle = dlopen( g_cccasterName.c_str(), RTLD_NOW );

    if ( g_cccasterHandle ) {
        // Clear any existing error
        dlerror();

        const auto l_initFunction = std::bit_cast< wrapper::initFunction_t >(
            dlsym( g_cccasterHandle, "init" ) );

        {
            const char* l_error = dlerror();

            if ( l_error != nullptr ) {
                std::cerr << std::format( "dlsym failed: {}\n", l_error );

                return false;
            }
        }

        std::cout << "CALLING INIT()\n";

        const bool l_result =
            l_initFunction( wrapper::makePatch, wrapper::makePatchByPattern,
                            wrapper::removePatch );

        return ( l_result );

    } else {
        std::cout << std::format( "CCCASTER FAILED TO LOAD: {}\n", dlerror() );

        return false;
    }

    return true;
}

auto detach() -> bool {
    std::cout << "WRAPPER DETACHED\n";

    if ( g_cccasterHandle ) {
        dlclose( g_cccasterHandle );
    }

    return true;
}

} // namespace

extern "C" auto APIENTRY DllMain( [[maybe_unused]] HMODULE _hModule,
                                  DWORD _ulReasonForCall,
                                  [[maybe_unused]] LPVOID _lpReserved )
    -> BOOL {
    switch ( _ulReasonForCall ) {
        case DLL_PROCESS_ATTACH: {
            const bool l_result = attach();

            if ( l_result ) {
                std::cout << "CCCASTER LOADED\n";
            } else {
                std::cerr << "CCCASTER FAILED TO INIT\n";
            }

            return l_result;

            break;
        }

        case DLL_PROCESS_DETACH: {
            return detach();

            break;
        }

        default: {
            break;
        }
    }

    return ( TRUE );
}
