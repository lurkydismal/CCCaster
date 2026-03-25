#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

using patch_t = struct patch {
    [[nodiscard]] patch( uintptr_t _address,
                         std::span< const std::byte > _bytes );

    ~patch();

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
