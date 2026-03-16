#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <dlfcn.h>

#include <iostream>

namespace {

constexpr const std::string g_cccasterName = "main.so";
void* g_cccasterHandle = nullptr;

auto attach() -> bool {
    std::cout << "WRAPPER ATTACHED\n";

    g_cccasterHandle = dlopen( g_cccasterName.c_str(), RTLD_NOW );

    if ( g_cccasterHandle ) {
        std::cout << "CCCASTER LOADED\n";

    } else {
        std::cout << "CCCASTER FAILED TO LOAD\n";

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
            return attach();

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
