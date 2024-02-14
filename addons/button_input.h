#if !defined( BUTTON_INPUT_H )
#define BUTTON_INPUT_H

#include <cstdint>

#if !defined( COMBINE_INPUT )
#define COMBINE_INPUT( BUTTONS, DIRECTION ) \
    ( uint16_t( ( ( uint16_t )DIRECTION ) | ( ( ( uint16_t )BUTTONS ) << 4 ) ) )
#endif

#if !defined( INLINE_INPUT )
#define INLINE_INPUT( INPUT )                                 \
    ( uint16_t( ( ( ( uint16_t )INPUT ) & 0xFFF0u ) >> 4 ) ), \
        ( uint16_t( ( ( uint16_t )INPUT ) & 0x000Fu ) )
#endif

enum button_t {
    NEUTRAL_BUTTON = 0x0,
    A = 0x0010,
    B = 0x0020,
    C = 0x0008,
    D = 0x0004,
    E = 0x0080,
    AB = 0x0040,
    FN1 = 0x0100, // Control dummy
    FN2 = 0x0200, // Training reset
    START = 0x0001,
    CONFIRM = 0x0400,
    CANCEL = 0x0800,
    FACING = 0x0002
};

#endif
