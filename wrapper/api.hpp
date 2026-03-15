#pragma once

#include <cstdint>
#include <cstring>
#include <expected>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

using memoryLock_t = struct memoryLock {
    memoryLock( uintptr_t _address, size_t _length );
    ~memoryLock();

    [[nodiscard]] constexpr auto ok() const -> bool { return _ok; }

    memoryLock( const memoryLock& ) = delete;
    memoryLock( memoryLock&& ) = default;
    auto operator=( const memoryLock& ) -> memoryLock& = delete;
    auto operator=( memoryLock&& ) -> memoryLock& = default;

private:
    bool _ok{};
    unsigned long _oldProtectionRules{};
    uintptr_t _address;
    size_t _length;
};

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

using patternByte_t = std::optional< std::byte >;
using pattern_t = std::vector< patternByte_t >;

[[nodiscard]] auto parsePattern( std::string_view _pattern )
    -> std::expected< pattern_t, std::string >;

[[nodiscard]] auto makePatches( uintptr_t _address,
                                std::span< const patternByte_t > _pattern,
                                std::span< const std::byte > _bytes )
    -> std::expected< std::vector< patch_t >, std::string >;

[[nodiscard]] auto makePatches( uintptr_t _address,
                                std::string_view _pattern,
                                std::span< const std::byte > _bytes )
    -> std::expected< std::vector< patch_t >, std::string >;
