#pragma once

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <optional>
#include <span>
#include <vector>

#include "patch.hpp"

using storage_t = struct storage {
    using handle_t = std::size_t;
    static constexpr handle_t g_invalidHandle = 0;

    storage() = default;
    ~storage() = default;

    explicit storage( handle_t _reserve );

    storage( const storage& ) = delete;
    auto operator=( const storage& ) -> storage& = delete;
    storage( storage&& ) = delete;
    auto operator=( storage&& ) -> storage& = delete;

    auto reserve( handle_t _count ) -> void;

    [[nodiscard]] auto addPatch( uintptr_t _address,
                                 const std::byte* _bytes,
                                 handle_t _bytesAmount ) -> handle_t;

    [[nodiscard]] auto addPatch( uintptr_t _address,
                                 std::span< const std::byte > _bytes )
        -> handle_t;

    [[nodiscard]] auto removePatch( handle_t _handle ) -> bool;

    [[nodiscard]] auto hasPatch( handle_t _handle ) const -> bool;

private:
    [[nodiscard]] auto _emplacePatch( uintptr_t _address,
                                      std::span< const std::byte > _bytes )
        -> handle_t;

    [[nodiscard]] auto _emplacePatchLocked(
        uintptr_t _address,
        std::span< const std::byte > _bytes ) -> handle_t;

private:
    mutable std::mutex _mutex;
    std::vector< std::optional< patch_t > > _slots;
    std::vector< handle_t > _free;
};
