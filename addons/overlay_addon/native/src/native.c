#include "native.h"

#include "stdfunc.h"

char* g_elementTypesAsString[] = { "rectangle", "text", "sprite", NULL };
element_t* g_elementsToRender;

/*
      A ------- B
      |         |
      |         |
      C --------D
*/
uint32_t drawRectangle( uint32_t _x,
                        uint32_t _y,
                        uint32_t _width,
                        uint32_t _height,
                        uint32_t _a,
                        uint32_t _b,
                        uint32_t _c,
                        uint32_t _d,
                        uint32_t _layerIndex );

uint32_t drawText( uint32_t _width,
                   uint32_t _height,
                   uint32_t _x,
                   uint32_t _y,
                   const char* _text,
                   uint32_t _alpha,
                   uint32_t _shade,
                   uint32_t _shade2,
                   uintptr_t* _fontAddress,
                   uint32_t _letterSpacing,
                   uint32_t _layerIndex,
                   char* _out );

uint32_t drawSprite( uint32_t _width,
                     uint32_t _directXDevice,
                     uintptr_t _textureAddress,
                     uint32_t _x,
                     uint32_t _y,
                     uint32_t _height,
                     uint32_t _textureX,
                     uint32_t _textureY,
                     uint32_t _textureWidth,
                     uint32_t _textureHeight,
                     uint32_t _flags,
                     uint32_t _unknown,
                     uint32_t _layerIndex );

uint32_t loadTextureFromMemory( char* _textureBuffer,
                                uint32_t _textureBufferLength,
                                char* _textureBuffer2,
                                uint32_t _textureBuffer2Length,
                                uint32_t _unknownFlags );

uint32_t getColorForRectangle( uint8_t _red,
                               uint8_t _green,
                               uint8_t _blue,
                               uint8_t _alpha ) {
    return ( ( _alpha << ( 8 * 3 ) ) + ( _red << ( 8 * 2 ) ) +
             ( _green << ( 8 * 1 ) ) + ( _blue ) );
}
