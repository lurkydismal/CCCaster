#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <concepts>
#include <string>
#include <type_traits>
#include <utility>

#include "logg.hpp"

namespace timer {

// RAII helper for measuring a block
using scoped_t = struct scoped {
    scoped( std::string _name, bool _enabled = true );
    ~scoped();
    auto setEnabled( bool _enabled ) -> void;

    scoped( const scoped& ) = default;
    scoped( scoped&& ) = delete;
    auto operator=( const scoped& ) -> scoped& = default;
    auto operator=( scoped&& ) -> scoped& = delete;

private:
    std::string _mName;
    bool _mEnabled{ true };
    LARGE_INTEGER _mFrequency{};
    LARGE_INTEGER _mStart{};
};

// Function template to measure any callable
template < typename Func >
auto measureBlock( Func&& _func, const std::string& _name = "Block" )
    -> std::invoke_result_t< Func > {
    using result_t = std::invoke_result_t< Func >;

    LARGE_INTEGER l_frequency{};
    LARGE_INTEGER l_start{};
    LARGE_INTEGER l_end{};

    QueryPerformanceFrequency( &l_frequency );
    QueryPerformanceCounter( &l_start );

    if constexpr ( std::is_void_v< result_t > ) {
        std::forward< Func >( _func )();
    } else {
        result_t l_result = std::forward< Func >( _func )();

        QueryPerformanceCounter( &l_end );

        double l_elapsedMs =
            static_cast< double >( l_end.QuadPart - l_start.QuadPart ) *
            1000.0 / l_frequency.QuadPart;
        double l_elapsedUs =
            static_cast< double >( l_end.QuadPart - l_start.QuadPart ) *
            1'000'000.0 / l_frequency.QuadPart;
        double l_elapsedS =
            static_cast< double >( l_end.QuadPart - l_start.QuadPart ) /
            l_frequency.QuadPart;

        logg::info( "[MeasureBlock] {}: {} ms, {} µs, {} s", _name,
                    l_elapsedMs, l_elapsedUs, l_elapsedS );

        return ( l_result );
    }

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

    logg::info( "[MeasureBlock] {}: {} ms, {} µs, {} s", _name, l_elapsedMs,
                l_elapsedUs, l_elapsedS );
}

} // namespace timer
