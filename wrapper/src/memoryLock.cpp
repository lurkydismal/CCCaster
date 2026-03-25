#include "memoryLock.hpp"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <exception>

#include "logg.hpp"

memoryLock::memoryLock( uintptr_t _address, size_t _length )
    : _address( _address ), _length( _length ) {
    logg::trace( "memoryLock::memoryLock addr={} len={}", _address, _length );

    if ( !_address || !_length ) {
        logg::error( "memoryLock::memoryLock invalid arguments" );
        std::terminate();
    }

    _ok = VirtualProtect( reinterpret_cast< void* >( _address ), _length,
                          PAGE_READWRITE, &_oldProtectionRules );

    if ( !_ok ) {
        logg::error( "Patch for {} : {} bytes failed.", _address, _length );
    } else {
        logg::debug( "memoryLock::memoryLock locked addr={} len={}", _address,
                     _length );
    }
}

memoryLock::~memoryLock() {
    if ( _oldProtectionRules ) {
        unsigned long l_temp = 0;

        logg::trace( "memoryLock::~memoryLock restore addr={} len={}", _address,
                     _length );

        if ( !VirtualProtect( reinterpret_cast< void* >( _address ), _length,
                              _oldProtectionRules, &l_temp ) ) {
            logg::error( "Patch removal for {} : {} bytes failed.", _address,
                         _length );
        } else {
            logg::debug( "memoryLock::~memoryLock restored addr={} len={}",
                         _address, _length );
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
