#include "timer.hpp"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <string>
#include <utility>

namespace timer {

// RAII helper for measuring a block
scoped::scoped( std::string _name ) : _mName( std::move( _name ) ) {
    QueryPerformanceFrequency( &_mFrequency );
    QueryPerformanceCounter( &_mStart );
}

scoped::~scoped() {
    LARGE_INTEGER l_end;
    QueryPerformanceCounter( &l_end );

    auto l_elapsed = ( l_end.QuadPart - _mStart.QuadPart );

    double l_ms = ( l_elapsed * 1000.0 ) / _mFrequency.QuadPart;
    double l_us = ( l_elapsed * 1'000'000.0 ) / _mFrequency.QuadPart;
    double l_s = ( l_elapsed * 1.0 ) / _mFrequency.QuadPart;

    logg::info( "[Timer] {}: ms, {} µs, {} s", _mName, l_ms, l_us, l_s );
}

} // namespace timer
