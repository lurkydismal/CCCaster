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
    if ( !_address || !_bytes || !_bytesAmount ) {
        return storage_t::g_invalidHandle;
    }

    const auto l_handle =
        g_patches.addPatch( _address, std::span{ _bytes, _bytesAmount } );

    if ( l_handle == storage_t::g_invalidHandle ) {
        logg::error( "Patch creation failed" );
    }

    return ( l_handle );
}

[[nodiscard]] auto removePatch( storage_t::handle_t _id ) -> bool {
    const bool l_result = g_patches.removePatch( _id );

    if ( !l_result ) {
        logg::error( "Patch removal failed" );
    }

    return ( l_result );
}

} // namespace wrapper
