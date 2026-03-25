#include "memoryLock.hpp"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <exception>

#include "logg.hpp"

memoryLock::memoryLock( uintptr_t _address, size_t _length )
    : _address( _address ), _length( _length ) {
    if ( !_address || !_length ) {
        std::terminate();
    }

    _ok = VirtualProtect( reinterpret_cast< void* >( _address ), _length,
                          PAGE_READWRITE, &_oldProtectionRules );

    if ( !_ok ) {
        logg::error( "Patch for {} : {} bytes failed.", _address, _length );
    }
}

memoryLock::~memoryLock() {
    if ( _oldProtectionRules ) {
        unsigned long l_temp = 0;

        if ( !VirtualProtect( reinterpret_cast< void* >( _address ), _length,
                              _oldProtectionRules, &l_temp ) ) {
            logg::error( "Patch removal for {} : {} bytes failed.", _address,
                         _length );
        }

        const int l_result = FlushInstructionCache(
            GetCurrentProcess(), reinterpret_cast< const void* >( _address ),
            _length );

        if ( !l_result ) {
            logg::error( "Instruction cache flush failed." );

            std::terminate();
        }
    }
}
