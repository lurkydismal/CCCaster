#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <concepts>
#include <string>

#include "logg.hpp"

namespace timer {

// RAII helper for measuring a block
using scoped_t = struct scoped {
    scoped( std::string _name );
    ~scoped();

    scoped( const scoped& ) = default;
    scoped( scoped&& ) = delete;
    auto operator=( const scoped& ) -> scoped& = default;
    auto operator=( scoped&& ) -> scoped& = delete;

private:
    std::string _mName;
    LARGE_INTEGER _mFrequency{};
    LARGE_INTEGER _mStart{};
};

// Function template to measure any callable
template < std::invocable R >
auto measureBlock( auto&& _func, const std::string& _name = "Block" ) -> R {
    LARGE_INTEGER l_frequency{};
    LARGE_INTEGER l_start{};
    LARGE_INTEGER l_end{};

    QueryPerformanceFrequency( &l_frequency );
    QueryPerformanceCounter( &l_start );

    R l_result = std::forward< decltype( _func ) >( _func )();

    QueryPerformanceCounter( &l_end );

    double l_elapsedMs =
        static_cast< double >( l_end.QuadPart - l_start.QuadPart ) * 1000.0 /
        l_frequency.QuadPart;
    double l_elapsedUs =
        static_cast< double >( l_end.QuadPart - l_start.QuadPart ) *
        1'000'000.0 / l_frequency.QuadPart;
    double l_elapsedS =
        static_cast< double >( l_end.QuadPart - l_start.QuadPart ) /
        l_frequency.QuadPart;

    logg::info( "[MeasureBlock] {}: ms, {} µs, {} s", _name, l_elapsedMs,
                l_elapsedUs, l_elapsedS );

    return ( l_result );
}

} // namespace timer
