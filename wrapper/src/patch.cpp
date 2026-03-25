#include "patch.hpp"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <algorithm>
#include <bit>
#include <exception>
#include <format>
#include <string>
#include <utility>

#include "logg.hpp"
#include "memoryLock.hpp"

namespace {

void printBytes( std::span< const std::byte > _bytes ) {
    std::string l_buffer = "[";

    for ( auto l_byte : _bytes ) {
        l_buffer += std::format( "{:X} ", static_cast< uint8_t >( l_byte ) );
    }

    l_buffer += "]";

    logg::trace( "[BYTES] {}", l_buffer );
}

void printBytes( uintptr_t _address, size_t _length ) {
    printBytes(
        std::span{ std::bit_cast< const std::byte* >( _address ), _length } );
}

} // namespace

[[nodiscard]] patch::patch( uintptr_t _address,
                            std::span< const std::byte > _bytes ) {
    if ( !_address || !_bytes.data() || !_bytes.size() ) {
        std::terminate();
    }

    const auto l_moduleBase =
        std::bit_cast< uintptr_t >( GetModuleHandle( nullptr ) );

    if ( l_moduleBase == _address ) {
        std::terminate();
    }

    uintptr_t l_address =
        ( _address > l_moduleBase ) ? _address : ( l_moduleBase + _address );

    const memoryLock_t l_lock( l_address, _bytes.size() );

    _ok = l_lock.ok();

    if ( !_ok ) {
        return;
    }

    this->_address = l_address;

    // Backup
    {
        printBytes( _bytes );

        this->_bytes.resize( _bytes.size() );

        std::ranges::copy(
            std::span( std::bit_cast< const std::byte* >( l_address ),
                       _bytes.size() ),
            this->_bytes.begin() );
    }

    // Write
    {
        std::ranges::copy( _bytes, std::bit_cast< std::byte* >( l_address ) );

        printBytes( l_address, _bytes.size() );
    }

    _ownsPatch = true;
}

patch::~patch() {
    _release();
}

patch::patch( patch&& _other ) {
    _moveFrom( std::move( _other ) );
}

auto patch::operator=( patch&& _other ) -> patch& {
    if ( this != &_other ) {
        _release();

        _moveFrom( std::move( _other ) );
    }

    return ( *this );
}

auto patch::_release() -> void {
    if ( !_ownsPatch ) {
        return;
    }

    const memoryLock_t l_lock( _address, _bytes.size() );

    if ( l_lock.ok() ) {
        std::ranges::copy( _bytes, std::bit_cast< std::byte* >( _address ) );
    }

    _ownsPatch = false;
    _ok = false;
    _address = 0;
    _bytes.clear();
}

// NOLINTNEXTLINE(cppcoreguidelines-rvalue-reference-param-not-moved)
auto patch::_moveFrom( patch&& _other ) -> void {
    _ok = std::exchange( _other._ok, false );
    _ownsPatch = std::exchange( _other._ownsPatch, false );
    _address = std::exchange( _other._address, 0 );
    _bytes = std::move( _other._bytes );
}
