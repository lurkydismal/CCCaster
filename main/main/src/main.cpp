#include <cstdlib>
#include <print>

#include "api.hpp"
#include "example.hpp"
#include "logg.hpp"

#if defined( __SANITIZE_LEAK__ )

#include <sanitizer/lsan_interface.h>

#endif

[[gnu::visibility( "default" )]] auto init(
    decltype( &wrapper::makePatch ) _makePatch,
    decltype( &wrapper::makePatchByPattern ) _makePatchByPattern,
    decltype( &wrapper::removePatch ) _removePatch ) -> bool {
    static_assert(
        std::is_same_v< decltype( init ),
                        std::remove_pointer_t< wrapper::initFunction_t > > );

    std::println( "{:#X} : {:#X} : '{:#X}'", ( uintptr_t )_makePatch,
                  ( uintptr_t )_makePatchByPattern, ( uintptr_t )_removePatch );

    example::printBuildType();

#if defined( __SANITIZE_LEAK__ )

    __lsan_do_leak_check();

#endif

    logg$error( "SUCCESS" );

    return ( EXIT_FAILURE );
}
