#pragma once

#include <cstdint>

enum AVAILABLE_FLAGS_KEYBOARD {
    SHOW_OVERLAY_NATIVE = 0b1,
    OVERLAY_IS_MAPPING_KEY = 0b1 << 2
};
