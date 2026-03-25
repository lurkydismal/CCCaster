#pragma once

#include <cstddef>
#include <cstdint>

#include "storage.hpp"

namespace wrapper {

[[nodiscard]] auto makePatch( uintptr_t _address,
                              const std::byte* _bytes,
                              size_t _bytesAmount ) -> storage_t::handle_t;
[[nodiscard]] auto removePatch( storage_t::handle_t _id ) -> bool;

using initFunction_t = auto ( * )( decltype( &wrapper::makePatch ),
                                   decltype( &wrapper::removePatch ),
                                   const char* _json,
                                   size_t _jsonLength ) -> bool;

} // namespace wrapper
