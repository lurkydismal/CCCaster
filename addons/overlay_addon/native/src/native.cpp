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
                              void* _fontAddress = ( void* )FONT2,
                              uint32_t _letterSpacing = 0,
                              uint32_t _layerIndex = 0,
                              char* _out = 0 );

extern "C" uint32_t drawSprite( uint32_t spriteWidth,
                                uint32_t dxdevice,
                                uint32_t texAddr,
                                uint32_t screenXAddr,
                                uint32_t screenYAddr,
                                uint32_t spriteHeight,
                                uint32_t texXAddr,
                                uint32_t texYAddr,
                                uint32_t texXSize,
                                uint32_t texYSize,
                                uint32_t flags,
                                uint32_t unk,
                                uint32_t _layerIndex );

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

    switch ( l_layerIndex == NULL ) {
        case 0: {
            drawRectangle( *l_x, *l_y, *l_width, *l_height, *l_a, *l_b, *l_c,
                           *l_d, *l_layerIndex );

            break;
        }

        default: {
            drawRectangle( *l_x, *l_y, *l_width, *l_height, *l_a, *l_b, *l_c,
                           *l_d );
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
    void** l_fontAddress = ( void** )_callbackArguments[ 8 ];
    uint32_t* l_letterSpacing = ( uint32_t* )_callbackArguments[ 9 ];
    uint32_t* l_layerIndex = ( uint32_t* )_callbackArguments[ 10 ];
    char** l_out = ( char** )_callbackArguments[ 11 ];

    switch ( ( l_width == NULL ) && ( l_height == NULL ) && ( l_x == NULL ) && ( l_y == NULL ) && ( l_text == NULL ) && ( l_alpha == NULL ) + ( l_shade == NULL ) + ( l_shade2 == NULL ) +
             ( l_fontAddress == NULL ) + ( l_letterSpacing == NULL ) +
             ( l_layerIndex == NULL ) + ( l_out == NULL ) ) {
        case 0: {
            drawText( *l_width, *l_height, *l_x, *l_y, *l_text, *l_alpha,
                      *l_shade, *l_shade2, *l_fontAddress, *l_letterSpacing,
                      *l_layerIndex, *l_out );

            break;
        }

        case 1: {
            drawText( *l_width, *l_height, *l_x, *l_y, *l_text, *l_alpha,
                      *l_shade, *l_shade2, *l_fontAddress, *l_letterSpacing,
                      *l_layerIndex );

            break;
        }

        case 2: {
            drawText( *l_width, *l_height, *l_x, *l_y, *l_text, *l_alpha,
                      *l_shade, *l_shade2, *l_fontAddress, *l_letterSpacing );

            break;
        }

        case 3: {
            drawText( *l_width, *l_height, *l_x, *l_y, *l_text, *l_alpha,
                      *l_shade, *l_shade2, *l_fontAddress );

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
            l_returnValue = ( 1 );
        }
    }

    return ( l_returnValue );
}

extern "C" uint16_t __declspec( dllexport )
    native$drawSprite( void** _callbackArguments ) {
    uint16_t l_returnValue = 0;

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

    if ( ( !l_textureBuffer2 ) && ( !l_textureBuffer2Length ) &&
         ( !l_unknownFlags ) ) {
        loadTextureFromMemory( *l_textureBuffer, *l_textureBufferLength );

    } else if ( ( !l_textureBuffer2Length ) && ( !l_unknownFlags ) ) {
        loadTextureFromMemory( *l_textureBuffer, *l_textureBufferLength,
                               *l_textureBuffer2 );

    } else if ( ( !l_unknownFlags ) ) {
        loadTextureFromMemory( *l_textureBuffer, *l_textureBufferLength,
                               *l_textureBuffer2, *l_textureBuffer2Length );

    } else {
        loadTextureFromMemory( *l_textureBuffer, *l_textureBufferLength,
                               *l_textureBuffer2, *l_textureBuffer2Length,
                               *l_unknownFlags );
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

    drawText( 24, 24, 200 - 1, 50, l_test.data(),
              0xff, // alpha
              0xff, // shade
              0x1f0, ( void* )FONT2, 0, 0, 0 );

    uint32_t l_width2 = 24;
    uint32_t l_height2 = 24;
    uint32_t l_x2 = ( 200 - 1 );
    uint32_t l_y2 = 50;
    const char* l_text = l_test.data();
    uint32_t l_alpha = 0xff;
    uint32_t l_shade = 0xff;
    uint32_t l_shade2 = 0x1f0;
    void* l_fontAddress = ( void* )FONT2;
    uint32_t l_letterSpacing = 0;
    uint32_t l_layerIndex2 = 0;
    char* l_out = 0;

    _useCallback( "native$drawText", 12, &l_width2, &l_height2, &l_x2, &l_y2,
                  &l_test, &l_alpha, &l_shade, &l_shade2, &l_fontAddress,
                  &l_letterSpacing, &l_layerIndex2, &l_out );

    //     drawSprite( 25, 0, *( uint32_t* )BUTTON_SPRITE_TEX, 200, 150, 25,
    //     0x19 * 6, 0,
    //                 0x19, 0x19, 0xFFFFFFFF, 0, 0x2cc );

    return ( 0 );
}
