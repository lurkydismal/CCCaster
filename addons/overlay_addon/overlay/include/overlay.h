#pragma once

#include <stdint.h>

enum AVAILABLE_FLAGS { SHOW_OVERLAY = 0b1 };

uint16_t overlayRegister( const char* _overlayName,
                          const char* const* _elementsOrder,
                          const char* _elementsDefaultSettings,
                          const uintptr_t* _elementsCallbackVariableReferences,
                          const char* _overlayDefaultHotkey );
