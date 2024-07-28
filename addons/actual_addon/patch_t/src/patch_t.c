#include "patch_t.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <stdio.h>
#include <stdlib.h>

#include "_useCallback.h"

patch_t patchCreate( uintptr_t _address,
                     uint8_t* _bytes,
                     const size_t _bytesLength ) {
    patch_t l_returnValue;
    l_returnValue.address = _address;
    l_returnValue.bytesBackup =
        ( uint8_t* )malloc( _bytesLength * sizeof( uint8_t ) );
    l_returnValue.bytesBackupLength = _bytesLength;

#if 0
    {
        char string[ 255 ] = { 0 };

        /*do something*/
        strcpy( string, "[" );
        for ( size_t i = 0; i < ( _bytesLength ); i++ ) {
            sprintf( &string[ strlen( string ) ], "%d, ", _bytes[ i ] );
        }

        sprintf( &string[ strlen( string ) ], "%d", _bytes[ 4 ] );
        strcat( string, "]" );

        const char l_message[] = "patchCreate( %lu, %p : [ %s ], %d )";
        const size_t l_messageLength = snprintf( NULL, 0, l_message, _address,
                                                 _bytes, string, _bytesLength );
        char l_buffer[ l_messageLength + 1 ];
        snprintf( l_buffer, sizeof( l_buffer ), l_message, _address, _bytes,
                  string, _bytesLength );

        _useCallback( "log$transaction$query", l_buffer,
                      ( void* )sizeof( l_buffer ) );
        _useCallback( "log$transaction$commit" );
    }
#endif

#if 0
    _useCallback( "log$transaction$query", l_returnValue.address, (void*)l_returnValue.bytesBackup, l_returnValue.bytesBackupLength );

    _useCallback( "log$transaction$commit" );
#endif

    {
        DWORD l_oldProtectionRules;

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

        free( _patch->bytesBackup );
    }

EXIT:
    return ( l_returnValue );
}
