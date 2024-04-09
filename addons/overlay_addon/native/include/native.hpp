#pragma once

#include <cstdint>

#define CC_SCREEN_WIDTH_ADDR \
    ( ( uint32_t* )0x54D048 ) // The actual width of the main viewport

// Addresses for drawText fonts
#define FONT0 ( 0x55D680 )
#define FONT1 ( 0x55D260 )
#define FONT2 ( 0x55DAA0 )

// Addresses for sprite textures
#define BUTTON_SPRITE_TEX ( 0x74d5e8 )

// typedef struct {
//     uint32_t x;
//     uint32_t y;
// } coordinates_t;

// struct size {
//     uint32_t width;
//     uint32_t height;
// };

// typedef struct {
//     uint32_t a;
//     uint32_t b;
//     uint32_t c;
//     uint32_t d;
// } colorsRectangle_t;
