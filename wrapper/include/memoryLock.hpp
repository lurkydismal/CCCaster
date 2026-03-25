#pragma once

#include <cstddef>
#include <cstdint>

using memoryLock_t = struct memoryLock {
    memoryLock( uintptr_t _address, size_t _length );

    ~memoryLock();

    memoryLock( const memoryLock& ) = delete;
    memoryLock( memoryLock&& ) = delete;
    auto operator=( const memoryLock& ) -> memoryLock& = delete;
    auto operator=( memoryLock&& ) -> memoryLock& = delete;

    [[nodiscard]] constexpr auto ok() const -> bool { return _ok; }

private:
    bool _ok{};
    unsigned long _oldProtectionRules{};
    uintptr_t _address;
    size_t _length;
};
