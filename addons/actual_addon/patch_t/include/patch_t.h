#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct PATCH {
    uintptr_t address;
    uint8_t* bytesBackup;
    size_t bytesBackupLength;
} patch_t;

#if defined( __cplusplus )

extern "C" {

#endif

patch_t patchCreate( uintptr_t _address,
                     uint8_t* _bytes,
                     const size_t _bytesLength );
bool patchRemove( patch_t* _patch );

#if defined( __cplusplus )
}

#endif
