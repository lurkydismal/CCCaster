#pragma once

#include <cstddef>
#include <cstdint>

#include "storage.hpp"

namespace wrapper {

auto setNoPatches( bool _value ) -> bool;

[[nodiscard]] auto makePatch( uintptr_t _address,
                              const std::byte* _bytes,
                              size_t _bytesAmount ) -> storage_t::handle_t;
[[nodiscard]] auto removePatch( storage_t::handle_t _id ) -> bool;
[[nodiscard]] auto readMemory( uintptr_t _address,
                               std::byte* _outBytes,
                               size_t _bytesAmount ) -> bool;
[[nodiscard]] auto writeMemory( uintptr_t _address,
                                const std::byte* _bytes,
                                size_t _bytesAmount,
                                bool _suspendProcess ) -> bool;

using apiVtable_t = struct apiVtable {
    decltype( &wrapper::makePatch ) makePatch;
    decltype( &wrapper::removePatch ) removePatch;
    decltype( &wrapper::readMemory ) readMemory;
    decltype( &wrapper::writeMemory ) writeMemory;
};

using initFunction_t = auto ( * )( const apiVtable_t* _vtable,
                                   const char* _json,
                                   size_t _jsonLength ) -> bool;

} // namespace wrapper
