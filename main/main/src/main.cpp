#include <cstdlib>
#include <print>
#include <span>

#include "example.hpp"
#include "logg.hpp"

#if defined( __SANITIZE_LEAK__ )

#include <sanitizer/lsan_interface.h>

#endif

auto main( int _argumentCount, char** _argumentVector ) -> int {
    std::println( "{}: '{}'", _argumentCount,
                  std::span( _argumentVector, _argumentCount ) );

    example::printBuildType();

#if defined( __SANITIZE_LEAK__ )

    __lsan_do_leak_check();

#endif

    logg$error( "SUCCESS" );

    return ( EXIT_FAILURE );
}
