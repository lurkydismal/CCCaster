#include "api.hpp"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <algorithm>
#include <bit>
#include <cstdint>
#include <cstring>
#include <format>
#include <iostream>
#include <iterator>
#include <list>
#include <variant>
#include <vector>

namespace {

using memoryLock_t = struct memoryLock {
    memoryLock( uintptr_t _address, size_t _length )
        : _address( _address ), _length( _length ) {
        _ok = VirtualProtect( std::bit_cast< void* >( _address ), _length,
                              PAGE_READWRITE, &_oldProtectionRules );

        if ( !_ok ) {
            std::cerr << std::format( "Patch for {} : {} bytes failed.\n",
                                      _address, _length );
        }
    }

    ~memoryLock() {
        if ( _oldProtectionRules ) {
            if ( !VirtualProtect( std::bit_cast< void* >( _address ), _length,
                                  _oldProtectionRules,
                                  &_oldProtectionRules ) ) {
                std::cerr << std::format(
                    "Patch removal for {} : {} bytes failed.\n", _address,
                    _length );
            }
        }
    }

    [[nodiscard]] constexpr auto ok() const -> bool { return _ok; }

    memoryLock( const memoryLock& ) = delete;
    memoryLock( memoryLock&& ) = default;
    auto operator=( const memoryLock& ) -> memoryLock& = delete;
    auto operator=( memoryLock&& ) -> memoryLock& = default;

private:
    bool _ok{};
    unsigned long _oldProtectionRules{};
    uintptr_t _address;
    size_t _length;
};

using patch_t = struct patch {
    [[nodiscard]] patch( uintptr_t _address,
                         std::span< const std::byte > _bytes ) {
        const auto l_moduleBase =
            std::bit_cast< uintptr_t >( GetModuleHandle( nullptr ) );

        uintptr_t l_address = ( l_moduleBase + _address );

        const memoryLock_t l_lock( l_address, _bytes.size() );

        _ok = l_lock.ok();

        if ( _ok ) {
            this->_address = l_address;
            this->_bytes.resize( _bytes.size() );

            // Backup
            std::ranges::copy(
                std::span( std::bit_cast< const std::byte* >( l_address ),
                           _bytes.size() ),
                this->_bytes.begin() );

            std::cout << ( "[" );
            for ( size_t l_i = 0; l_i < this->_bytes.size(); ++l_i ) {
                std::cout << std::format(
                    "{:X}", static_cast< uint8_t >( this->_bytes[ l_i ] ) );
                if ( l_i + 1 < this->_bytes.size() )
                    std::cout << ( ", " );
            }
            std::cout << ( "]\n" );

            // Write
            std::ranges::copy( _bytes,
                               std::bit_cast< std::byte* >( l_address ) );
        }
    }

    ~patch() {
        const memoryLock_t l_lock( _address, _bytes.size() );

        _ok = l_lock.ok();

        if ( _ok ) {
            // Write
            std::ranges::copy( _bytes,
                               std::bit_cast< std::byte* >( _address ) );
        }
    }

    [[nodiscard]] constexpr auto ok() const -> bool { return _ok; }
    [[nodiscard]] constexpr auto address() const -> uintptr_t {
        return _address;
    }

    patch( const patch& ) = delete;
    patch( patch&& ) = default;
    auto operator=( const patch& ) -> patch& = delete;
    auto operator=( patch&& ) -> patch& = default;

private:
    bool _ok{};
    uintptr_t _address{};
    std::vector< std::byte > _bytes{};
};

[[nodiscard]] constexpr auto nibble( char _character )
    -> std::optional< char > {
    const bool l_isDigit = ( ( _character >= '0' ) && ( _character <= '9' ) );
    const bool l_isUpper = ( ( _character >= 'A' ) && ( _character <= 'F' ) );
    const bool l_isLower = ( ( _character >= 'a' ) && ( _character <= 'f' ) );

#if defined( DEBUG )

    stdfunc::assert( l_isDigit || l_isUpper || l_isLower, "Character: '{}'",
                     _character );

#endif

    if ( !( l_isDigit || l_isUpper || l_isLower ) ) {
        return ( std::nullopt );
    }

    return ( static_cast< uint8_t >(
        ( l_isDigit ) ? ( _character - '0' )
                      : ( ( _character & ~0x20 ) - 'A' +
                          0xA ) // Fold lowercase to uppercase
        ) );
}

using patternByte_t = std::optional< std::byte >;
using pattern_t = std::vector< patternByte_t >;

[[nodiscard]] constexpr auto parseByteToken( std::string_view _token )
    -> std::expected< patternByte_t, std::string > {
    if ( _token == "?" || _token == "??" ) {
        return ( std::nullopt );
    }

    if ( _token.size() != 2 ) {
        return ( std::unexpected( "Invalid argument" ) );
    }

    const auto l_hi = nibble( _token[ 0 ] );
    const auto l_lo = nibble( _token[ 1 ] );

    if ( ( l_hi.value() < 0 ) || ( l_lo.value() < 0 ) ) {
        return ( std::unexpected( "Invalid argument" ) );
    }

    return ( static_cast< std::byte >( ( l_hi.value() << 4 ) | l_lo.value() ) );
}

[[nodiscard]] auto parsePattern( std::string_view _pattern )
    -> std::expected< pattern_t, std::string > {
    pattern_t l_result{};

    std::size_t l_pos{};

    while ( l_pos < _pattern.size() ) {
        while ( ( l_pos < _pattern.size() ) &&
                std::isspace( _pattern[ l_pos ] ) ) {
            ++l_pos;
        }

        if ( l_pos >= _pattern.size() ) {
            break;
        }

        const auto l_begin = l_pos;

        while ( ( l_pos < _pattern.size() ) &&
                !std::isspace( _pattern[ l_pos ] ) ) {
            ++l_pos;
        }

        const auto l_token = _pattern.substr( l_begin, l_pos - l_begin );
        const auto l_parsed = parseByteToken( l_token );

        if ( !l_parsed ) {
            return ( std::unexpected( l_parsed.error() ) );
        }

        l_result.push_back( l_parsed.value() );
    }

    return ( l_result );
}

[[nodiscard]] constexpr auto countConcreteBytes(
    std::span< const patternByte_t > _pattern ) -> std::size_t {
    std::size_t l_count{};

    for ( const auto& l_item : _pattern ) {
        if ( l_item.has_value() ) {
            ++l_count;
        }
    }

    return ( l_count );
}

[[nodiscard]] constexpr auto countRuns(
    std::span< const patternByte_t > _pattern ) -> std::size_t {
    std::size_t l_runs{};
    bool l_inRun{ false };

    for ( const auto& l_item : _pattern ) {
        if ( l_item.has_value() ) {
            if ( !l_inRun ) {
                ++l_runs;
                l_inRun = true;
            }
        } else {
            l_inRun = false;
        }
    }

    return ( l_runs );
}

[[nodiscard]] auto makePatches( uintptr_t _address,
                                std::span< const patternByte_t > _pattern,
                                std::span< const std::byte > _bytes )
    -> std::expected< std::vector< patch_t >, std::string > {
    if ( countConcreteBytes( _pattern ) != _bytes.size() ) {
        return ( std::unexpected( "Invalid argument" ) );
    }

    std::vector< patch_t > l_patches{};
    l_patches.reserve( countRuns( _pattern ) );

    std::size_t l_byteIndex{};
    std::size_t l_index{};

    while ( l_index < _pattern.size() ) {
        while ( ( l_index < _pattern.size() ) &&
                !_pattern[ l_index ].has_value() ) {
            ++l_index;
        }

        if ( l_index >= _pattern.size() ) {
            break;
        }

        const auto l_patchBegin = l_index;

        std::vector< std::byte > l_patchBytes{};
        while ( ( l_index < _pattern.size() ) &&
                _pattern[ l_index ].has_value() ) {
            l_patchBytes.push_back( _bytes[ l_byteIndex ] );
            ++l_byteIndex;
            ++l_index;
        }

        l_patches.emplace_back(
            _address + static_cast< uintptr_t >( l_patchBegin ), l_patchBytes );
    }

    return ( l_patches );
}

[[nodiscard]] auto makePatches( uintptr_t _address,
                                std::string_view _pattern,
                                std::span< const std::byte > _bytes )
    -> std::expected< std::vector< patch_t >, std::string > {
    const auto l_parsed = parsePattern( _pattern );

    if ( !l_parsed ) {
        return ( std::unexpected( l_parsed.error() ) );
    }

    return ( makePatches( _address, l_parsed.value(), _bytes ) );
}

std::list< std::variant< patch_t, std::vector< patch_t > > > g_patches;

} // namespace

namespace wrapper {

[[nodiscard]] auto makePatch( uintptr_t _address,
                              std::span< const std::byte > _bytes ) -> size_t {
    g_patches.emplace_back( ::patch_t{ _address, _bytes } );

    const auto l_it = std::prev( g_patches.end() );

    return std::distance( g_patches.begin(), l_it );
}

[[nodiscard]] auto makePatchByPattern( uintptr_t _address,
                                       std::string_view _pattern,
                                       std::span< const std::byte > _bytes )
    -> std::expected< size_t, std::string > {
    auto l_patches = makePatches( _address, _pattern, _bytes );

    if ( !l_patches ) {
        return std::unexpected( l_patches.error() );
    }

    g_patches.emplace_back( std::move( l_patches.value() ) );

    const auto l_it = std::prev( g_patches.end() );

    return std::distance( g_patches.begin(), l_it );
}

void removePatch( size_t _id ) {
    if ( _id > g_patches.size() ) {
        // TODO: Report error
        return;
    }

    g_patches.erase( std::next( g_patches.begin(), _id ) );
}

} // namespace wrapper