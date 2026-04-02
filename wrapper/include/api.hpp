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
[[nodiscard]] auto createDetour( uintptr_t _targetAddress,
                                 uintptr_t _detourAddress,
                                 uintptr_t* _outTrampolineAddress,
                                 bool _suspendProcess ) -> storage_t::handle_t;
[[nodiscard]] auto removeDetour( storage_t::handle_t _id ) -> bool;

using apiVtable_t = struct apiVtable {
    decltype( &wrapper::makePatch ) makePatch;
    decltype( &wrapper::removePatch ) removePatch;
    decltype( &wrapper::readMemory ) readMemory;
    decltype( &wrapper::writeMemory ) writeMemory;
    decltype( &wrapper::createDetour ) createDetour;
    decltype( &wrapper::removeDetour ) removeDetour;
};

using initFunction_t = auto ( * )( const apiVtable_t* _vtable,
                                   const char* _json,
                                   size_t _jsonLength ) -> bool;

} // namespace wrapper
