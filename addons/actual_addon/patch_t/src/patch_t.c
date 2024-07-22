#include "patch_t.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <stdlib.h>

#if defined( __cplusplus )

extern "C" {

#endif

patch_t patchCreate( uintptr_t _address,
                     uint8_t* _bytes,
                     const size_t _bytesLength ) {
    patch_t l_returnValue;
    l_returnValue.address = _address;
    l_returnValue.bytesBackup =
        ( uint8_t* )malloc( _bytesLength * sizeof( uint8_t ) );
    l_returnValue.bytesBackupLength = _bytesLength;

    {
        DWORD l_oldProtectionRules;

        if ( !VirtualProtect( ( void* )( _address ), _bytesLength,
                              PAGE_READWRITE, &l_oldProtectionRules ) ) {
            l_returnValue.bytesBackup = NULL;
            l_returnValue.bytesBackupLength = 0;

            goto EXIT;
        }

        memcpy( ( l_returnValue.bytesBackup ), ( void* )( _address ),
                _bytesLength );

        memcpy( ( void* )( _address ), ( &_bytes ), _bytesLength );

        if ( !VirtualProtect( ( void* )( _address ), _bytesLength,
                              l_oldProtectionRules, &l_oldProtectionRules ) ) {
            l_returnValue.bytesBackup = NULL;
            l_returnValue.bytesBackupLength = 0;

            goto EXIT;
        }
    }

EXIT:
    return ( l_returnValue );
}

bool patchRemove( patch_t* _patch ) {
    bool l_returnValue = true;

    {
        DWORD l_oldProtectionRules;

        if ( !VirtualProtect( ( void* )( _patch->address ),
                              _patch->bytesBackupLength, PAGE_READWRITE,
                              &l_oldProtectionRules ) ) {
            l_returnValue = false;

            goto EXIT;
        }

        memcpy( ( void* )( _patch->address ), &( _patch->bytesBackup )[ 0 ],
                _patch->bytesBackupLength );

        if ( !VirtualProtect( ( void* )( _patch->address ),
                              _patch->bytesBackupLength, l_oldProtectionRules,
                              &l_oldProtectionRules ) ) {
            l_returnValue = false;

            goto EXIT;
        }
    }

EXIT:
    return ( l_returnValue );
}

#if defined( __cplusplus )
}

#endif
