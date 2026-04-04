#pragma once

#include <cstddef>
#include <cstdint>

namespace wrappercfg {

/**
 * Raw shared-memory payload produced by the launcher.
 */
struct data_t {
    size_t size;
    char value[];
};

/**
 * Wrapper-specific runtime configuration parsed from launcher JSON.
 */
struct wrapperData_t {
    uint8_t verbose{};
    bool trace{};
    bool timings{};
    bool no_patches{};
};

} // namespace wrappercfg
