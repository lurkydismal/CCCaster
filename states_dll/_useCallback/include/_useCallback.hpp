#pragma once

#include <cstdint>
#include <string>

typedef uint16_t( __cdecl* useCallbackFunction_t )( const char* _callbackName,
                                                    void** _callbackArguments );
extern useCallbackFunction_t g_useCallback;

uint16_t _useCallback( std::string const& _callbackName,
                       const size_t _callbackArgumentsCount = 0,
                       ... );
