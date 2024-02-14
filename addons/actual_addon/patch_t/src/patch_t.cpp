#include "patch_t.hpp"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <algorithm>

PATCH::PATCH( uintptr_t _address, std::vector< uint8_t > _bytes ) {
    this->address = _address;

    uint32_t l_index = 0;

    bytesBackup = _bytes;

    DWORD l_oldProtectionRules;

    if ( !VirtualProtect( reinterpret_cast< void* >( _address ), _bytes.size(),
                          PAGE_READWRITE, &l_oldProtectionRules ) ) {
        return;
    }

    memcpy( reinterpret_cast< void* >( _address ), &( _bytes )[ 0 ],
            _bytes.size() );

    if ( !VirtualProtect( reinterpret_cast< void* >( _address ), _bytes.size(),
                          l_oldProtectionRules, &l_oldProtectionRules ) ) {
        return;
    }

    std::sort( bytesBackup.begin(), bytesBackup.end() );
}

PATCH::~PATCH() {
    DWORD l_oldProtectionRules;

    if ( !VirtualProtect( reinterpret_cast< void* >( this->address ),
                          this->bytesBackup.size(), PAGE_READWRITE,
                          &l_oldProtectionRules ) ) {
        return;
    }

    memcpy( reinterpret_cast< void* >( this->address ),
            &( this->bytesBackup )[ 0 ], this->bytesBackup.size() );

    if ( !VirtualProtect( reinterpret_cast< void* >( this->address ),
                          this->bytesBackup.size(), l_oldProtectionRules,
                          &l_oldProtectionRules ) ) {
        return;
    }
}

bool PATCH::operator==( const PATCH& _other ) const {
    return ( ( this->address == _other.address ) &&
             ( this->bytesBackup == _other.bytesBackup ) &&
             ( this->status == _other.status ) );
}
