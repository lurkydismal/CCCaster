#pragma once

#include <cstdint>
#include <vector>

typedef struct PATCH {
    uintptr_t address = 0;
    std::vector< uint8_t > bytesBackup = {};
    bool status = false;

    PATCH( void );
    PATCH( uintptr_t _address, std::vector< uint8_t > _bytes );

    ~PATCH();

    bool operator==( const PATCH& _other ) const;
    bool operator!( void ) const;

    bool apply( uintptr_t _address, std::vector< uint8_t > _bytes );
} patch_t;
