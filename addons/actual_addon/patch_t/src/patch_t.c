#include "patch_t.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <memoryapi.h>
#undef PAGE_READWRITE

#include <stdlib.h>
#include <string.h>

#include "_useCallback.h"
#include "stdfunc.h"

#define PAGE_READWRITE 4

patch_t patchCreate( uintptr_t _address,
                     uint8_t* _bytes,
                     const size_t _bytesLength ) {
    patch_t l_returnValue;
    l_returnValue.address = _address;
    l_returnValue.bytesBackup = ( uint8_t* )createArray( sizeof( uint8_t ) );

    preallocateArray( ( void*** )( &( l_returnValue.bytesBackup ) ),
                      _bytesLength );

    unsigned long int l_oldProtectionRules;

    if ( !VirtualProtect( ( void* )( _address ), _bytesLength, PAGE_READWRITE,
                          &l_oldProtectionRules ) ) {
        goto FAIL;
    }

    {
        memcpy( arrayFirstElementPointer( l_returnValue.bytesBackup ),
                ( void* )( _address ), _bytesLength );

        memcpy( ( void* )( _address ), ( &_bytes )[ 0 ], _bytesLength );
    }

    if ( !VirtualProtect( ( void* )( _address ), _bytesLength,
                          l_oldProtectionRules, &l_oldProtectionRules ) ) {
        goto FAIL;
    }

EXIT:
    return ( l_returnValue );

FAIL:
    free( l_returnValue.bytesBackup );

    l_returnValue.bytesBackup = NULL;

    goto EXIT;
}

bool patchRemove( patch_t* _patch ) {
    bool l_returnValue = false;

    if ( _patch->bytesBackup != NULL ) {
        unsigned long int l_oldProtectionRules;

        if ( !VirtualProtect( ( void* )( _patch->address ),
                              arrayLength( _patch->bytesBackup ),
                              PAGE_READWRITE, &l_oldProtectionRules ) ) {
            goto EXIT;
        }

        memcpy( ( void* )( _patch->address ), &( _patch->bytesBackup )[ 0 ],
                arrayLength( _patch->bytesBackup ) );

        if ( !VirtualProtect( ( void* )( _patch->address ),
                              arrayLength( _patch->bytesBackup ),
                              l_oldProtectionRules, &l_oldProtectionRules ) ) {
            goto EXIT;
        }

        free( _patch->bytesBackup );

        l_returnValue = true;
    }

EXIT:
    return ( l_returnValue );
}
