#include "patch_t.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <memoryapi.h>
#undef PAGE_READWRITE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "_useCallback.h"

#define PAGE_READWRITE 4

patch_t patchCreate( uintptr_t _address,
                     uint8_t* _bytes,
                     const size_t _bytesLength ) {
    patch_t l_returnValue;
    l_returnValue.address = _address;
    l_returnValue.bytesBackup =
        ( uint8_t* )malloc( _bytesLength * sizeof( uint8_t ) );
    l_returnValue.bytesBackupLength = _bytesLength;

    {
        unsigned long int l_oldProtectionRules;

        if ( !VirtualProtect( ( void* )( _address ), _bytesLength,
                              PAGE_READWRITE, &l_oldProtectionRules ) ) {
            l_returnValue.bytesBackup = NULL;
            l_returnValue.bytesBackupLength = 0;

            goto EXIT;
        }

        memcpy( l_returnValue.bytesBackup, ( void* )( _address ),
                _bytesLength );

        memcpy( ( void* )( _address ), ( &_bytes )[ 0 ], _bytesLength );

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
        unsigned long int l_oldProtectionRules;

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

        free( _patch->bytesBackup );
    }

EXIT:
    return ( l_returnValue );
}
