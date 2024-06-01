#include "native.hpp"

#include <string>

#include "_useCallback.hpp"

namespace {

/*
      A ------- B
      |         |
      |         |
      C --------D
*/
extern "C" uint32_t drawRectangle( uint32_t _x,
                                   uint32_t _y,
                                   uint32_t _width,
                                   uint32_t _height,
                                   uint32_t _a,
                                   uint32_t _b,
                                   uint32_t _c,
                                   uint32_t _d,
                                   uint32_t _layerIndex );

extern "C" uint32_t drawText( uint32_t _width,
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

extern "C" uint32_t drawSprite( uint32_t _width,
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

extern "C" uint32_t loadTextureFromMemory( char* _textureBuffer,
                                           uint32_t _textureBufferLength,
                                           char* _textureBuffer2,
                                           uint32_t _textureBuffer2Length,
                                           uint32_t _unknownFlags );

} // namespace

extern "C" uint16_t __declspec( dllexport )
    native$getColorForRectangle( void** _callbackArguments ) {
    uint16_t l_returnValue = 0;

    uint8_t* l_alpha = ( uint8_t* )_callbackArguments[ 0 ];
    uint8_t* l_red = ( uint8_t* )_callbackArguments[ 1 ];
    uint8_t* l_green = ( uint8_t* )_callbackArguments[ 2 ];
    uint8_t* l_blue = ( uint8_t* )_callbackArguments[ 3 ];

    *( uint32_t* )_callbackArguments[ 4 ] =
        ( ( *l_alpha << 24 ) + ( *l_red << 16 ) + ( *l_green << 8 ) +
          ( *l_blue ) );

    return ( l_returnValue );
}

extern "C" uint16_t __declspec( dllexport )
    native$drawRectangle( void** _callbackArguments ) {
    uint16_t l_returnValue = 0;

    uint32_t* l_x = ( uint32_t* )_callbackArguments[ 0 ];
    uint32_t* l_y = ( uint32_t* )_callbackArguments[ 1 ];
    uint32_t* l_width = ( uint32_t* )_callbackArguments[ 2 ];
    uint32_t* l_height = ( uint32_t* )_callbackArguments[ 3 ];
    uint32_t* l_a = ( uint32_t* )_callbackArguments[ 4 ];
    uint32_t* l_b = ( uint32_t* )_callbackArguments[ 5 ];
    uint32_t* l_c = ( uint32_t* )_callbackArguments[ 6 ];
    uint32_t* l_d = ( uint32_t* )_callbackArguments[ 7 ];
    uint16_t* l_layerIndex = ( uint16_t* )_callbackArguments[ 8 ];

    drawRectangle( *l_x, *l_y, *l_width, *l_height, *l_a, *l_b, *l_c, *l_d,
                   *l_layerIndex );

    return ( l_returnValue );
}

extern "C" uint16_t __declspec( dllexport )
    native$drawText( void** _callbackArguments ) {
    uint16_t l_returnValue = 0;

    uint32_t* l_width = ( uint32_t* )_callbackArguments[ 0 ];
    uint32_t* l_height = ( uint32_t* )_callbackArguments[ 1 ];
    uint32_t* l_x = ( uint32_t* )_callbackArguments[ 2 ];
    uint32_t* l_y = ( uint32_t* )_callbackArguments[ 3 ];
    const char** l_text = ( const char** )_callbackArguments[ 4 ];
    uint32_t* l_alpha = ( uint32_t* )_callbackArguments[ 5 ];
    uint32_t* l_shade = ( uint32_t* )_callbackArguments[ 6 ];
    uint32_t* l_shade2 = ( uint32_t* )_callbackArguments[ 7 ];
    uintptr_t* l_fontAddress = ( uintptr_t* )_callbackArguments[ 8 ];
    uint32_t* l_letterSpacing = ( uint32_t* )_callbackArguments[ 9 ];
    uint32_t* l_layerIndex = ( uint32_t* )_callbackArguments[ 10 ];
    char** l_out = ( char** )_callbackArguments[ 11 ];

    drawText( *l_width, *l_height, *l_x, *l_y, *l_text, *l_alpha, *l_shade,
              *l_shade2, l_fontAddress, *l_letterSpacing, *l_layerIndex,
              *l_out );

    return ( l_returnValue );
}

extern "C" uint16_t __declspec( dllexport )
    native$drawSprite( void** _callbackArguments ) {
    uint16_t l_returnValue = 0;

    uint32_t* l_width = ( uint32_t* )_callbackArguments[ 0 ];
    uint32_t* l_directXDevice = ( uint32_t* )_callbackArguments[ 1 ];
    uintptr_t* l_textureAddress = ( uintptr_t* )_callbackArguments[ 2 ];
    uint32_t* l_x = ( uint32_t* )_callbackArguments[ 3 ];
    uint32_t* l_y = ( uint32_t* )_callbackArguments[ 4 ];
    uint32_t* l_height = ( uint32_t* )_callbackArguments[ 5 ];
    uint32_t* l_textureX = ( uint32_t* )_callbackArguments[ 6 ];
    uint32_t* l_textureY = ( uint32_t* )_callbackArguments[ 7 ];
    uint32_t* l_textureWidth = ( uint32_t* )_callbackArguments[ 8 ];
    uint32_t* l_textureHeight = ( uint32_t* )_callbackArguments[ 9 ];
    uint32_t* l_flags = ( uint32_t* )_callbackArguments[ 10 ];
    uint32_t* l_unknown = ( uint32_t* )_callbackArguments[ 11 ];
    uint32_t* l_layerIndex = ( uint32_t* )_callbackArguments[ 12 ];

    drawSprite( *l_width, *l_directXDevice, *l_textureAddress, *l_x, *l_y,
                *l_height, *l_textureX, *l_textureY, *l_textureWidth,
                *l_textureHeight, *l_flags, *l_unknown, *l_layerIndex );

    return ( l_returnValue );
}

extern "C" uint16_t __declspec( dllexport )
    native$loadTextureFromMemory( void** _callbackArguments ) {
    uint16_t l_returnValue = 0;

    char** l_textureBuffer = ( char** )_callbackArguments[ 0 ];
    uint32_t* l_textureBufferLength = ( uint32_t* )_callbackArguments[ 1 ];
    char** l_textureBuffer2 = ( char** )_callbackArguments[ 2 ];
    uint32_t* l_textureBuffer2Length = ( uint32_t* )_callbackArguments[ 3 ];
    uint32_t* l_unknownFlags = ( uint32_t* )_callbackArguments[ 4 ];

    loadTextureFromMemory( *l_textureBuffer, *l_textureBufferLength,
                           *l_textureBuffer2, *l_textureBuffer2Length,
                           *l_unknownFlags );

    return ( l_returnValue );
}
