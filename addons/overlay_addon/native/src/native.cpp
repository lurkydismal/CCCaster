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
                                   uint32_t _layerIndex = 0x2ff );

extern "C" uint32_t drawText( uint32_t _width,
                              uint32_t _height,
                              uint32_t _x,
                              uint32_t _y,
                              const char* _text,
                              uint32_t _alpha = 0xff,
                              uint32_t _shade = 0xff,
                              uint32_t _shade2 = 0x1f0,
                              uintptr_t* _fontAddress = ( uintptr_t* )FONT2,
                              uint32_t _letterSpacing = 0,
                              uint32_t _layerIndex = 0,
                              char* _out = 0 );

extern "C" uint32_t drawSprite( uint32_t _width,
                                uint32_t _directXDevice,
                                uint32_t _textureAddress,
                                uint32_t _x,
                                uint32_t _y,
                                uint32_t _height,
                                uint32_t _textureX,
                                uint32_t _textureY,
                                uint32_t _textureWidth,
                                uint32_t _textureHeight,
                                uint32_t _flags = 0xFFFFFFFF,
                                uint32_t _unknown = 0,
                                uint32_t _layerIndex = 0x2cc );

extern "C" uint32_t loadTextureFromMemory( char* _textureBuffer,
                                           uint32_t _textureBufferLength,
                                           char* _textureBuffer2 = 0,
                                           uint32_t _textureBuffer2Length = 0,
                                           uint32_t _unknownFlags = 0 );

} // namespace

inline uint32_t getColorRectangle( uint8_t _alpha,
                                   uint8_t _red,
                                   uint8_t _green,
                                   uint8_t _blue ) {
    return ( ( _alpha << 24 ) + ( _red << 16 ) + ( _green << 8 ) + ( _blue ) );
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

    switch ( ( l_x == NULL ) + ( l_y == NULL ) + ( l_width == NULL ) +
             ( l_height == NULL ) + ( l_a == NULL ) + ( l_b == NULL ) +
             ( l_c == NULL ) + ( l_d == NULL ) + ( l_layerIndex == NULL ) ) {
        case 0: {
            drawRectangle( *l_x, *l_y, *l_width, *l_height, *l_a, *l_b, *l_c,
                           *l_d, *l_layerIndex );

            break;
        }

        case 1: {
            drawRectangle( *l_x, *l_y, *l_width, *l_height, *l_a, *l_b, *l_c,
                           *l_d );

            break;
        }

        default: {
            l_returnValue = EINVAL;
        }
    }

    return ( l_returnValue );
}

// extern "C" uint16_t __declspec( dllexport )
//     _native$drawRectangle( void** _callbackArguments ) {
//     uint16_t l_returnValue = 0;

//     coordinates_t* l_coordinates = ( coordinates_t* )_callbackArguments[ 0 ];
//     struct size* l_size = ( struct size* )_callbackArguments[ 1 ];
//     colorsRectangle_t* l_colorsRectangle = ( colorsRectangle_t*
//     )_callbackArguments[ 2 ]; uint16_t* l_layerIndex = ( uint16_t*
//     )_callbackArguments[ 3 ];

//     native$drawRectangle( &(l_coordinates->x), &(l_coordinates->y),
//     &(l_size->width), &(l_size->height), &(l_colorsRectangle->a),
//     &(l_colorsRectangle->b), &(l_colorsRectangle->c),
//                    &(l_colorsRectangle->d), l_layerIndex );

//     return ( l_returnValue );
// }

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

    switch ( ( l_width == NULL ) + ( l_height == NULL ) + ( l_x == NULL ) +
             ( l_y == NULL ) + ( l_text == NULL ) + ( l_alpha == NULL ) +
             ( l_shade == NULL ) + ( l_shade2 == NULL ) +
             ( l_fontAddress == NULL ) + ( l_letterSpacing == NULL ) +
             ( l_layerIndex == NULL ) + ( l_out == NULL ) ) {
        case 0: {
            drawText( *l_width, *l_height, *l_x, *l_y, *l_text, *l_alpha,
                      *l_shade, *l_shade2, l_fontAddress, *l_letterSpacing,
                      *l_layerIndex, *l_out );

            break;
        }

        case 1: {
            drawText( *l_width, *l_height, *l_x, *l_y, *l_text, *l_alpha,
                      *l_shade, *l_shade2, l_fontAddress, *l_letterSpacing,
                      *l_layerIndex );

            break;
        }

        case 2: {
            drawText( *l_width, *l_height, *l_x, *l_y, *l_text, *l_alpha,
                      *l_shade, *l_shade2, l_fontAddress, *l_letterSpacing );

            break;
        }

        case 3: {
            drawText( *l_width, *l_height, *l_x, *l_y, *l_text, *l_alpha,
                      *l_shade, *l_shade2, l_fontAddress );

            break;
        }

        case 4: {
            drawText( *l_width, *l_height, *l_x, *l_y, *l_text, *l_alpha,
                      *l_shade, *l_shade2 );

            break;
        }

        case 5: {
            drawText( *l_width, *l_height, *l_x, *l_y, *l_text, *l_alpha,
                      *l_shade );

            break;
        }

        case 6: {
            drawText( *l_width, *l_height, *l_x, *l_y, *l_text, *l_alpha );

            break;
        }

        case 7: {
            drawText( *l_width, *l_height, *l_x, *l_y, *l_text );

            break;
        }

        default: {
            l_returnValue = EINVAL;
        }
    }

    return ( l_returnValue );
}

extern "C" uint16_t __declspec( dllexport )
    native$drawSprite( void** _callbackArguments ) {
    uint16_t l_returnValue = 0;

    uint32_t* l_width = ( uint32_t* )_callbackArguments[ 0 ];
    uint32_t* l_directXDevice = ( uint32_t* )_callbackArguments[ 1 ];
    uint32_t* l_textureAddress = ( uint32_t* )_callbackArguments[ 2 ];
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

    switch ( ( l_width == NULL ) + ( l_directXDevice == NULL ) +
             ( l_textureAddress == NULL ) + ( l_x == NULL ) + ( l_y == NULL ) +
             ( l_height == NULL ) + ( l_textureX == NULL ) +
             ( l_textureY == NULL ) + ( l_textureWidth == NULL ) +
             ( l_textureHeight == NULL ) + ( l_flags == NULL ) +
             ( l_unknown == NULL ) + ( l_layerIndex == NULL ) ) {
        case 0: {
            drawSprite( *l_width, *l_directXDevice, *l_textureAddress, *l_x,
                        *l_y, *l_height, *l_textureX, *l_textureY,
                        *l_textureWidth, *l_textureHeight, *l_flags, *l_unknown,
                        *l_layerIndex );

            break;
        }

        case 1: {
            drawSprite( *l_width, *l_directXDevice, *l_textureAddress, *l_x,
                        *l_y, *l_height, *l_textureX, *l_textureY,
                        *l_textureWidth, *l_textureHeight, *l_flags,
                        *l_unknown );

            break;
        }

        case 2: {
            drawSprite( *l_width, *l_directXDevice, *l_textureAddress, *l_x,
                        *l_y, *l_height, *l_textureX, *l_textureY,
                        *l_textureWidth, *l_textureHeight, *l_flags );

            break;
        }

        case 3: {
            drawSprite( *l_width, *l_directXDevice, *l_textureAddress, *l_x,
                        *l_y, *l_height, *l_textureX, *l_textureY,
                        *l_textureWidth, *l_textureHeight );

            break;
        }

        default: {
            l_returnValue = EINVAL;
        }
    }

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

    switch ( ( l_textureBuffer == NULL ) + ( l_textureBuffer2Length == NULL ) +
             ( l_textureBuffer2 == NULL ) + ( l_textureBuffer2Length == NULL ) +
             ( l_unknownFlags == NULL ) ) {
        case 0: {
            loadTextureFromMemory( *l_textureBuffer, *l_textureBufferLength,
                                   *l_textureBuffer2, *l_textureBuffer2Length,
                                   *l_unknownFlags );

            break;
        }

        case 1: {
            loadTextureFromMemory( *l_textureBuffer, *l_textureBufferLength,
                                   *l_textureBuffer2, *l_textureBuffer2Length );

            break;
        }

        case 2: {
            loadTextureFromMemory( *l_textureBuffer, *l_textureBufferLength,
                                   *l_textureBuffer2 );

            break;
        }

        case 3: {
            loadTextureFromMemory( *l_textureBuffer, *l_textureBufferLength );

            break;
        }

        default: {
            l_returnValue = EINVAL;
        }
    }

    return ( l_returnValue );
}

extern "C" uint16_t __declspec( dllexport )
    drawNative( void** _callbackArguments ) {
    const uint32_t l_color = getColorRectangle( 0xff, 30, 30, 0xff );

    // drawRectangle( 640 - 200 - 10, 50, 200 + 5, 300 - 50, l_color, l_color,
    //                l_color, l_color, 0x2ff );

    uint32_t l_x = ( 640 - 600 - 10 );
    uint32_t l_y = 50;
    uint32_t l_width = ( 200 + 5 );
    uint32_t l_height = ( 300 - 50 );
    uint32_t l_layerIndex = 0x2ff;

    _useCallback( "native$drawRectangle", 9, &l_x, &l_y, &l_width, &l_height,
                  &l_color, &l_color, &l_color, &l_color );

    // coordinates_t l_coordinates = { l_x, l_y };
    // struct size l_size = { l_width, l_height };
    // colorsRectangle_t l_colorsRectangle = { l_color, l_color, l_color,
    // l_color };

    // _useCallback( "_native$drawRectangle", 9, &l_coordinates, &l_size,
    //               &l_colorsRectangle );

    std::string l_test = "ttt";

    uint32_t l_width2 = 24;
    uint32_t l_height2 = 24;
    uint32_t l_x2 = ( 300 - 1 );
    uint32_t l_y2 = 50;
    const char* l_text = l_test.data();
    uint32_t l_alpha = 0xff;
    uint32_t l_shade = 0xff;
    uint32_t l_shade2 = 0x1f0;
    uintptr_t* l_fontAddress = ( uintptr_t* )FONT2;
    uint32_t l_letterSpacing = 0;
    uint32_t l_layerIndex2 = 0;
    char* l_out = 0;

    // drawText( l_width2, l_height2, l_x2, l_y2, l_text, l_alpha, l_shade,
    //           l_shade2, l_fontAddress, l_letterSpacing, l_layerIndex2, l_out
    //           );

    _useCallback( "native$drawText", 12, &l_width2, &l_height2, &l_x2, &l_y2,
                  &l_text, &l_alpha, &l_shade, &l_shade2, l_fontAddress,
                  &l_letterSpacing, &l_layerIndex2, &l_out );

    //     drawSprite( 25, 0, *( uint32_t* )BUTTON_SPRITE_TEX, 200, 150, 25,
    //     0x19 * 6, 0,
    //                 0x19, 0x19, 0xFFFFFFFF, 0, 0x2cc );

    return ( 0 );
}
