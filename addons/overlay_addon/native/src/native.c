#include "native.h"

char* g_elementTypesAsString[] = { "rectangle", "text", "sprite", NULL };
element_t*** g_overlaysToRender;
char** g_overlayHotkeys;
char** g_overlayNames;

uint32_t getColorForRectangle( uint8_t _red,
                               uint8_t _green,
                               uint8_t _blue,
                               uint8_t _alpha ) {
    return ( ( _alpha << ( 8 * 3 ) ) + ( _red << ( 8 * 2 ) ) +
             ( _green << ( 8 * 1 ) ) + ( _blue ) );
}
