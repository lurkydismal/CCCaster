#pragma once

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

// Return a sorted list with increasing order
template < typename T >
inline T sorted( const T& list ) {
    std::vector< typename T::const_pointer > ptrs;
    ptrs.reserve( list.size() );
    for ( const auto& x : list )
        ptrs.push_back( &x );

    std::sort( ptrs.begin(), ptrs.end(),
               []( typename T::const_pointer a, typename T::const_pointer b ) {
                   return ( *a ) < ( *b );
               } );

    T sorted;
    sorted.reserve( ptrs.size() );
    for ( const auto& x : ptrs )
        sorted.push_back( *x );
    return sorted;
}

// Return a sorted list with provided comparison function
template < typename T, typename F >
inline T sorted( const T& list, const F& compare ) {
    std::vector< typename T::const_pointer > ptrs;
    ptrs.reserve( list.size() );
    for ( const auto& x : list )
        ptrs.push_back( &x );

    std::sort(
        ptrs.begin(), ptrs.end(),
        [ & ]( typename T::const_pointer a, typename T::const_pointer b ) {
            return compare( *a, *b );
        } );

    T sorted;
    sorted.reserve( ptrs.size() );
    for ( const auto& x : ptrs )
        sorted.push_back( *x );
    return sorted;
}

// Clamp a value to a range
template < typename T >
inline T clamped( T value, T min, T max ) {
    if ( value < min )
        return min;
    else if ( value > max )
        return max;
    else
        return value;
}

// True if x is a power of two
inline bool isPowerOfTwo( uint32_t x ) {
    return ( x != 0 ) && ( ( x & ( x - 1 ) ) == 0 );
}

// Random 30 character ID
inline std::string generateRandomId() {
    std::string randId;

    for ( int i = 0; i < 10; ++i ) {
        randId += ( 'A' + ( rand() % 26 ) );
        randId += ( 'a' + ( rand() % 26 ) );
        randId += ( '0' + ( rand() % 10 ) );
    }

    return randId;
}

// This increases somewhat linearly then slows down as i approaches count.
// Returns 1 - ( x - 1 )^2, where x = i / count
inline double getNegativeQuadraticScale( size_t i, size_t count ) {
    return 1.0 - std::pow( ( double( i ) / count ) - 1, 2.0 );
}

// Return the incremented value of x
template < typename T >
inline T incremented( T x ) {
    ++x;
    return x;
}

// Hash hash_combine function
namespace std {

template < class T >
void hash_combine( size_t& seed, const T& v ) {
    hash< T > hasher;
    seed ^= hasher( v ) + 0x9e3779b9 + ( seed << 6 ) + ( seed >> 2 );
}

} // namespace std
