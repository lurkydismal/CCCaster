#include "api.hpp"

#include <cstdint>
#include <cstring>
#include <span>

#include "logg.hpp"
#include "storage.hpp"

namespace {

storage_t g_patches;

} // namespace

namespace wrapper {

[[nodiscard]] auto makePatch( uintptr_t _address,
                              const std::byte* _bytes,
                              size_t _bytesAmount ) -> storage_t::handle_t {
    logg::trace( "wrapper::makePatch addr={} size={}", _address, _bytesAmount );

    if ( !_address || !_bytes || !_bytesAmount ) {
        logg::warning( "wrapper::makePatch invalid arguments" );
        return storage_t::g_invalidHandle;
    }

    const auto l_handle =
        g_patches.addPatch( _address, std::span{ _bytes, _bytesAmount } );

    if ( l_handle == storage_t::g_invalidHandle ) {
        logg::error( "Patch creation failed" );
    } else {
        logg::info( "wrapper::makePatch handle={}", l_handle );
    }

    return ( l_handle );
}

[[nodiscard]] auto removePatch( storage_t::handle_t _id ) -> bool {
    logg::trace( "wrapper::removePatch handle={}", _id );

    const bool l_result = g_patches.removePatch( _id );

    if ( !l_result ) {
        logg::error( "Patch removal failed" );
    } else {
        logg::info( "wrapper::removePatch removed handle={}", _id );
    }

    return ( l_result );
}

} // namespace wrapper
