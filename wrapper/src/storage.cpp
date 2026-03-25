#include "storage.hpp"

#include "logg.hpp"

storage::storage( handle_t _reserve ) {
    logg::debug( "storage::storage reserve {}", _reserve );
    reserve( _reserve );
}

auto storage::reserve( handle_t _count ) -> void {
    std::scoped_lock l_lock( _mutex );

    logg::debug( "storage::reserve {}", _count );

    _slots.reserve( _count );
    _free.reserve( _count );
}

[[nodiscard]] auto storage::addPatch( uintptr_t _address,
                                      const std::byte* _bytes,
                                      handle_t _bytesAmount ) -> handle_t {
    logg::trace( "storage::addPatch addr={} size={}", _address, _bytesAmount );
    return ( _emplacePatch( _address, std::span{ _bytes, _bytesAmount } ) );
}

[[nodiscard]] auto storage::addPatch( uintptr_t _address,
                                      std::span< const std::byte > _bytes )
    -> handle_t {
    logg::trace( "storage::addPatch addr={} size={}", _address, _bytes.size() );

    std::scoped_lock l_lock( _mutex );

    return ( _emplacePatchLocked( _address, _bytes ) );
}

[[nodiscard]] auto storage::removePatch( handle_t _handle ) -> bool {
    logg::trace( "storage::removePatch handle={}", _handle );

    if ( _handle == g_invalidHandle ) {
        logg::warning( "storage::removePatch invalid handle" );
        return ( false );
    }

    std::scoped_lock l_lock( _mutex );

    const handle_t l_index = ( _handle - 1 );

    if ( l_index >= _slots.size() ) {
        logg::warning( "storage::removePatch handle out of range" );
        return ( false );
    }

    auto& l_slot = _slots[ l_index ];

    if ( !l_slot.has_value() ) {
        logg::warning( "storage::removePatch empty slot" );
        return ( false );
    }

    l_slot.reset();

    _free.push_back( l_index );

    logg::info( "storage::removePatch removed handle={}", _handle );

    return ( true );
}

[[nodiscard]] auto storage::hasPatch( handle_t _handle ) const -> bool {
    if ( _handle == g_invalidHandle ) {
        return ( false );
    }

    std::scoped_lock l_lock( _mutex );

    const handle_t l_index = ( _handle - 1 );

    if ( l_index >= _slots.size() ) {
        return ( false );
    }

    return ( _slots[ l_index ].has_value() );
}

[[nodiscard]] auto storage::_emplacePatch( uintptr_t _address,
                                           std::span< const std::byte > _bytes )
    -> handle_t {
    std::scoped_lock l_lock( _mutex );

    return ( _emplacePatchLocked( _address, _bytes ) );
}

[[nodiscard]] auto storage::_emplacePatchLocked(
    uintptr_t _address,
    std::span< const std::byte > _bytes ) -> handle_t {
    const bool l_reuse = ( !_free.empty() );

    handle_t l_index{};

    if ( l_reuse ) {
        l_index = _free.back();
        _free.pop_back();

        logg::debug( "storage::_emplacePatchLocked reuse slot {}", l_index );
    } else {
        l_index = _slots.size();
        _slots.emplace_back();

        logg::debug( "storage::_emplacePatchLocked append slot {}", l_index );
    }

    auto& l_slot = _slots[ l_index ];

    // Construct in-place. No temporary patch_t move here.
    l_slot.emplace( _address, _bytes );

    if ( !l_slot->ok() ) {
        logg::error( "storage::_emplacePatchLocked patch creation failed" );

        l_slot.reset();

        if ( l_reuse ) {
            _free.push_back( l_index );
        } else {
            _slots.pop_back();
        }

        return ( g_invalidHandle );
    }

    // 1-based handle: 0 stays invalid.
    const handle_t l_handle = ( l_index + 1 );

    logg::info( "storage::_emplacePatchLocked created handle={}", l_handle );

    return ( l_handle );
}
