#pragma once

#include <stdint.h>

enum AVAILABLE_FLAGS { SHOW_OVERLAY = 0b1 };

uint16_t overlayRegister( const char* _overlayName,
                          const char* const* const* _overlay,
                          uintptr_t* _overlayValueReferences,
                          const char* _overlayDefaultHotkey );
