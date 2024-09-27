#pragma once

#include <stdint.h>

enum AVAILABLE_FLAGS { SHOW_OVERLAY = 0b1 };

uint16_t overlayRegister( const char* _overlayName,
                          const char* const* _overlayItems,
                          const char* _overlay,
                          uintptr_t* _overlayValueReferences,
                          const char* _overlayDefaultHotkey );
