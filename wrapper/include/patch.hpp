#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

using patch_t = struct patch {
    [[nodiscard]] patch( uintptr_t _address,
                         std::span< const std::byte > _bytes );

    ~patch();

    patch( const patch& ) = delete;
    patch( patch&& );
    auto operator=( const patch& ) -> patch& = delete;
    auto operator=( patch&& ) -> patch&;

    [[nodiscard]] constexpr auto ok() const -> bool { return ( _ok ); }
    [[nodiscard]] constexpr auto address() const -> uintptr_t {
        return ( _address );
    }

private:
    auto _release() -> void;
    auto _moveFrom( patch&& _other ) -> void;

private:
    bool _ok{};
    bool _ownsPatch{};
    uintptr_t _address{};
    std::vector< std::byte > _bytes{};
};
