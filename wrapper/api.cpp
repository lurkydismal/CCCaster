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
#include <vector>

memoryLock_t::memoryLock( uintptr_t _address, size_t _length )
    : _address( _address ), _length( _length ) {
    _ok = VirtualProtect( std::bit_cast< void* >( _address ), _length,
                          PAGE_READWRITE, &_oldProtectionRules );

    if ( !_ok ) {
        std::cerr << std::format( "Patch for {} : {} bytes failed.\n", _address,
                                  _length );
    }
}

memoryLock_t::~memoryLock() {
    if ( _oldProtectionRules ) {
        if ( !VirtualProtect( std::bit_cast< void* >( _address ), _length,
                              _oldProtectionRules, &_oldProtectionRules ) ) {
            std::cerr << std::format(
                "Patch removal for {} : {} bytes failed.\n", _address,
                _length );
        }
    }
}

[[nodiscard]] patch_t::patch( uintptr_t _address,
                              std::span< const std::byte > _bytes ) {
    const memoryLock_t l_lock( _address, _bytes.size() );

    _ok = l_lock.ok();

    if ( _ok ) {
        this->_address = _address;
        this->_bytes.resize( _bytes.size() );

        // Backup
        std::ranges::copy(
            std::span( std::bit_cast< const std::byte* >( _address ),
                       _bytes.size() ),
            this->_bytes.begin() );

        // Write
        std::ranges::copy( _bytes, std::bit_cast< std::byte* >( _address ) );
    }
}

patch_t::~patch() {
    const memoryLock_t l_lock( _address, _bytes.size() );

    _ok = l_lock.ok();

    if ( _ok ) {
        // Write
        std::ranges::copy( _bytes, std::bit_cast< std::byte* >( _address ) );
    }
}

[[nodiscard]] static constexpr auto nibble( char _character )
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

[[nodiscard]] static auto parseByteToken( std::string_view _token )
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

[[nodiscard]] static auto countConcreteBytes(
    std::span< const patternByte_t > _pattern ) -> std::size_t {
    std::size_t l_count{};

    for ( const auto& l_item : _pattern ) {
        if ( l_item.has_value() ) {
            ++l_count;
        }
    }

    return ( l_count );
}

[[nodiscard]] static auto countRuns( std::span< const patternByte_t > _pattern )
    -> std::size_t {
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
