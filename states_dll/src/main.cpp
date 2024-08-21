#include <errno.h>
#include <stdint.h>

#include <string>
#include <vector>

#include "bytell_map.hpp"

typedef uint16_t addonCallbackFunction_t( void** );

ska::bytell_hash_map< std::string, std::vector< addonCallbackFunction_t* > >
    g_callbackFunctionAddresses;

extern "C" bool addCallbacks( const char* _callbackName,
                              const size_t _functionCount,
                              const uintptr_t* _functionAddresses,
                              const bool _overwrite ) {
    bool l_returnValue = true;
    std::vector< addonCallbackFunction_t* > l_callbacks;

    for ( size_t _functionIndex = 0; _functionIndex < _functionCount;
          _functionIndex++ ) {
        l_callbacks.emplace_back(
            ( reinterpret_cast< addonCallbackFunction_t* >(
                _functionAddresses[ _functionIndex ] ) ) );
    }

    l_returnValue =
        g_callbackFunctionAddresses.emplace( _callbackName, l_callbacks )
            .second;

    if ( ( _overwrite ) && ( !l_returnValue ) ) {
        g_callbackFunctionAddresses[ _callbackName ] = l_callbacks;

        l_returnValue = true;
    }

    return ( l_returnValue );
}

extern "C" uint16_t useCallback( const char* _callbackName,
                                 void** _callbackArguments ) {
    uint16_t l_returnValue = 0;
    const auto l_callbacks = g_callbackFunctionAddresses.find( _callbackName );

    if ( l_callbacks == g_callbackFunctionAddresses.end() ) {
        l_returnValue = ENODATA;

    } else {
        for ( const addonCallbackFunction_t* _callback : l_callbacks->second ) {
            const uint16_t l_result = _callback( _callbackArguments );

            if ( l_result ) {
                l_returnValue = l_result;

                break;
            }
        }
    }

    return ( l_returnValue );
}
