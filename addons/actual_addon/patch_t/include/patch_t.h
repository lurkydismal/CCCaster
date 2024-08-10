#pragma once

#include <stdbool.h>
#include <stdint.h>

#define MAKE_PATCH( _address, ... )                              \
    do {                                                         \
        uint8_t l_patch[] = __VA_ARGS__;                         \
        patchCreate( _address, l_patch, ( sizeof( l_patch ) ) ); \
    } while ( 0 )

#define MAKE_PATCH_WITH_RETURN( _returnVariableName, _address, ... ) \
    do {                                                             \
        uint8_t l_patch[] = __VA_ARGS__;                             \
        _returnVariableName =                                        \
            patchCreate( _address, l_patch, ( sizeof( l_patch ) ) ); \
    } while ( 0 )

typedef struct {
    uintptr_t address;
    uint8_t* bytesBackup;
    size_t bytesBackupLength;
} patch_t;

patch_t patchCreate( uintptr_t _address,
                     uint8_t* _bytes,
                     const size_t _bytesLength );
bool patchRemove( patch_t* _patch );
