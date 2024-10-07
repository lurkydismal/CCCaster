#include "overlay.h"

#include "native.h"

char* g_elementTypesAsString[] = { "rectangle", "text", "sprite", NULL };
element_t*** g_overlaysToRender;
char** g_overlayHotkeys;
char** g_overlayNames;

uint32_t inline getColorForRectangle( color_t _color ) {
    return ( _getColorForRectangle( _color.red, _color.green, _color.blue,
                                    _color.alpha ) );
}
