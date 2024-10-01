#pragma once

#include <stdint.h>

#define DEFAULT_ELEMENT_PARAMETERS { NONE, { 0, 0 }, { 0, 0 }, 0, 0, 0, 0, "" }
#define MAX_ELEMENTS_TO_RENDER_IN_TOTAL 1000
#define SCREEN_WIDTH ( 0x54D048 ) // The actual width of the main viewport

// Addresses for drawText fonts
#define FONT0 ( 0x55D680 )
#define FONT1 ( 0x55D260 )
#define FONT2 ( 0x55DAA0 )

// Addresses for sprite textures
#define BUTTON_TEXTURE ( 0x74d5e8 )

typedef struct {
    uint32_t a;
    uint32_t b;
    uint32_t c;
    uint32_t d;
} colorsForRectangle_t;

typedef struct {
    int32_t x;
    int32_t y;
} coordinates_t;

struct size {
    uint32_t width;
    uint32_t height;
};

enum elementType { RECTANGLE, TEXT, SPRITE, NONE };

extern char* g_elementTypesAsString[];

typedef struct {
    enum elementType type;
    coordinates_t coordinates;
    struct size size;
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    uint8_t alpha;
    char* text;
} element_t;

extern element_t* g_elementsToRender;

uint32_t getColorForRectangle( uint8_t _red,
                               uint8_t _green,
                               uint8_t _blue,
                               uint8_t _alpha );
