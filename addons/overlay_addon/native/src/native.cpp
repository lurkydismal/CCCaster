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
extern "C" int32_t drawRectangle( int32_t _x,
                                  int32_t _y,
                                  int32_t _width,
                                  int32_t _height,
                                  int32_t _a,
                                  int32_t _b,
                                  int32_t _c,
                                  int32_t _d,
                                  int32_t _layerIndex = 0x2ff );

extern "C" int32_t drawText( int32_t _width,
                             int32_t _height,
                             int32_t _x,
                             int32_t _y,
                             const char* _text,
                             int32_t _alpha = 0xff,
                             int32_t _shade = 0xff,
                             int32_t _shade2 = 0x1f0,
                             void* _fontAddress = ( void* )FONT2,
                             int32_t _letterSpacing = 0,
                             int32_t _layerIndex = 0,
                             char* out = 0 );

extern "C" int32_t drawSprite( int32_t spriteWidth,
                               int32_t dxdevice,
                               int32_t texAddr,
                               int32_t screenXAddr,
                               int32_t screenYAddr,
                               int32_t spriteHeight,
                               int32_t texXAddr,
                               int32_t texYAddr,
                               int32_t texXSize,
                               int32_t texYSize,
                               int32_t flags,
                               int32_t unk,
                               int32_t _layerIndex );

extern "C" int32_t loadTextureFromMemory( char* _textureBuffer,
                                          uint32_t _textureBufferLength,
                                          char* _textureBuffer2 = 0,
                                          uint32_t _textureBuffer2Length = 0,
                                          uint32_t _unknownFlags = 0 );

} // namespace

inline uint32_t getRectangleColor( uint8_t _alpha,
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

    if ( !l_layerIndex ) {
        drawRectangle( *l_x, *l_y, *l_width, *l_height, *l_a, *l_b, *l_c,
                       *l_d );

    } else {
        drawRectangle( *l_x, *l_y, *l_width, *l_height, *l_a, *l_b, *l_c, *l_d,
                       *l_layerIndex );
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
    const uint32_t l_color = getRectangleColor( 0xff, 30, 30, 0xff );

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
    // colorsRectangle_t l_colorsRectangle = { l_color, l_color, l_color, l_color };

    // _useCallback( "_native$drawRectangle", 9, &l_coordinates, &l_size,
    //               &l_colorsRectangle );

    //     std::string l_test = "ttt";

    //     drawText( 24, 24, 200 - 1, 50, const_cast< char* >( l_test.data() ),
    //               0xff, // alpha
    //               0xff, // shade
    //               0x1f0, ( void* )FONT2, 0, 0, 0 );

    //     drawSprite( 25, 0, *( int* )BUTTON_SPRITE_TEX, 200, 150, 25, 0x19 *
    //     6, 0,
    //                 0x19, 0x19, 0xFFFFFFFF, 0, 0x2cc );

    return ( 0 );
}
