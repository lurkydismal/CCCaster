#pragma once

#include <stdint.h>

#if defined( __cplusplus )

extern "C" {

#endif

typedef uint16_t( __cdecl* useCallbackFunction_t )( const char* _callbackName,
                                                    void** _callbackArguments );
extern useCallbackFunction_t g_useCallback;

uint16_t _useCallback( const char* _callbackName,
                       const size_t _callbackArgumentsCount,
                       ... );

#if defined( __cplusplus )
}

#endif
