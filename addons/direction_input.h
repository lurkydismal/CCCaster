#if !defined( DIRECTION_INPUT_H )
#define DIRECTION_INPUT_H

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

enum direction_t {
    NEUTRAL_DIRECTION = 0x0,
    UP = 8,
    RIGHT = 6,
    DOWN = 2,
    LEFT = 4
};

#endif
