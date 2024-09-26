#include "addon_callbacks.h"

#include "_useCallback.h"
#include "stdfunc.h"

useCallbackFunction_t g_useCallback = NULL;

uint16_t __declspec( dllexport ) IDirect3D9Ex$CreateDevice(
    void** _callbackArguments ) {
    _useCallbackInitialize();

    return ( 0 );
}

uint16_t __declspec( dllexport ) overlay$register( void** _callbackArguments ) {
    uint16_t l_returnValue = 0;

    return ( l_returnValue );
}

#if 0
uint16_t __declspec( dllexport ) keyboard$getInput$end(
    void** _callbackArguments ) {
    if ( g_framesPassed > 7 ) {
        if ( g_activeFlagsOverlay & OVERLAY_IS_MAPPING_KEY ) {
            std::set< std::string >* _activeMappedKeys =
                ( std::set< std::string >* )_callbackArguments[ 0 ];
            std::set< uint8_t >* _activeKeys =
                ( std::set< uint8_t >* )_callbackArguments[ 1 ];

            if ( _activeMappedKeys->find( "B" ) != _activeMappedKeys->end() ) {
                g_activeFlagsOverlay &= ~OVERLAY_IS_MAPPING_KEY;

                g_framesPassed = 0;
            }

            if ( !_activeKeys->empty() ) {
                if ( g_keyboardLayout.find( *( _activeKeys->begin() ) ) !=
                     g_keyboardLayout.end() ) {
                    uint16_t l_index = 0;

                    for ( const auto& _item : g_jsonControlsKeyboard.items() ) {
                        if ( l_index == g_menuCursorIndex ) {
                            g_jsonControlsKeyboard[ std::to_string(
                                *( _activeKeys->begin() ) ) ] =
                                _item.value().template get< std::string >();

                            g_jsonControlsKeyboard.erase( _item.key() );

                            const std::string l_controlsConfigFileName =
                                ( std::string(
                                      CONTROLS_PREFERENCES_FILE_NAME ) +
                                  std::string( "." ) +
                                  std::string(
                                      CONTROLS_PREFERENCES_FILE_EXTENSION ) );

                            json l_jsonControls = {
                                { "keyboard", g_jsonControlsKeyboard },
                            };

                            std::string buffer =
                                l_jsonControls.dump( 4 ) + std::string( "\n" );

                            writeFile( l_controlsConfigFileName.c_str(),
                                       buffer.c_str() );

                            break;
                        }

                        l_index++;
                    }
                }

                g_menuCursorIndex = ( g_jsonControlsKeyboard.size() - 1 );

                g_activeFlagsOverlay &= ~OVERLAY_IS_MAPPING_KEY;

                g_framesPassed = 0;
            }

        } else if ( g_activeFlagsOverlay & SHOW_NATIVE ) {
            std::set< std::string >* _activeMappedKeys =
                ( std::set< std::string >* )_callbackArguments[ 0 ];

            if ( _activeMappedKeys->find( "8" ) != _activeMappedKeys->end() ) {
                g_menuCursorIndex--;

                if ( g_menuCursorIndex >
                     ( g_jsonControlsKeyboard.size() - 1 ) ) {
                    g_menuCursorIndex = ( g_jsonControlsKeyboard.size() - 1 );
                }

                g_framesPassed = 0;
            } else if ( _activeMappedKeys->find( "2" ) !=
                        _activeMappedKeys->end() ) {
                g_menuCursorIndex++;

                if ( g_menuCursorIndex >
                     ( g_jsonControlsKeyboard.size() - 1 ) ) {
                    g_menuCursorIndex = 0;
                }

                g_framesPassed = 0;
            }

            if ( _activeMappedKeys->find( "A" ) != _activeMappedKeys->end() ) {
                g_activeFlagsOverlay |= OVERLAY_IS_MAPPING_KEY;

                g_framesPassed = 0;

            } else if ( _activeMappedKeys->find( "B" ) !=
                        _activeMappedKeys->end() ) {
                g_activeFlagsOverlay &= ~OVERLAY_IS_MAPPING_KEY;

                g_framesPassed = 0;
            }
        }
    }

    return ( 0 );
}

uint16_t __declspec( dllexport ) extraDrawCallback(
    void** _callbackArguments ) {
    uint16_t l_returnValue = 0;

    static bool l_animationNeeded = true;

    if ( g_activeFlagsOverlay & SHOW_NATIVE ) {
        const uint8_t l_maxFramesPerSecond = 60;
        const float l_transitionTimeInSeconds = 0.2;
        static int32_t l_overlayY = 0;
        RECT l_tempRectangle;
        static uint32_t l_windowHeight = 0;

        if ( GetWindowRect( g_hFocusWindow, &l_tempRectangle ) ) {
            uint32_t l_tempHeight =
                ( l_tempRectangle.bottom - l_tempRectangle.top );

            if ( ( l_overlayY < 0 ) && ( l_windowHeight ) &&
                 ( l_windowHeight != l_tempHeight ) ) {
                l_overlayY = ( ( l_tempHeight + l_overlayY ) * -1 );
            }

            l_windowHeight = l_tempHeight;

            if ( l_animationNeeded ) {
                l_overlayY = ( l_windowHeight * -1 );

                l_animationNeeded = false;
            }
        }

        struct rectangle {
            uint16_t layer;
            colorsForRectangle_t colorsForRectangle;
            coordinates_t coordinates;
            struct size size;
        };

        struct text {
            uint16_t layer;
            uint8_t alpha;
            uint16_t shade;
            uint16_t shade2;
            coordinates_t coordinates;
            struct size size;
            const std::string content;
        };

        std::vector< struct rectangle > l_rectangles;
        std::vector< struct text > l_texts;

        // Logic
        {
            // Background
            {
                const uint8_t l_alpha = 65;
                const uint8_t l_red = 0xDA;
                const uint8_t l_green = 0xDA;
                const uint8_t l_blue = 0xDA;
                uint32_t l_color = 0;
                struct rectangle l_background;

                _useCallback( "native$getColorForRectangle", &l_alpha,
                              &l_red, &l_green, &l_blue, &l_color );

                l_background.colorsForRectangle = { l_color, l_color, l_color,
                                                    l_color };

                l_background.layer = ( 0 );

                l_background.size = { *( uint32_t* )SCREEN_WIDTH,
                                      l_windowHeight };

                if ( l_overlayY < 0 ) {
                    l_overlayY += ( l_background.size.height /
                                    ( l_transitionTimeInSeconds *
                                      l_maxFramesPerSecond ) );
                }

                if ( l_overlayY > 0 ) {
                    l_overlayY = ( -1 * ( l_background.size.height /
                                          ( l_transitionTimeInSeconds *
                                            l_maxFramesPerSecond ) ) );
                }

                l_background.coordinates = { 0, l_overlayY };

                l_rectangles.push_back( l_background );
            }

            // UI
            {
                struct rectangle l_textBackground;

                // Text background
                {
                    const uint8_t l_alpha = 0xFF;
                    const uint8_t l_red = 0;
                    const uint8_t l_green = 0;
                    const uint8_t l_blue = 0;
                    uint32_t l_color = 0;

                    _useCallback( "native$getColorForRectangle", &l_alpha,
                                  &l_red, &l_green, &l_blue, &l_color );

                    l_textBackground.colorsForRectangle = { l_color, l_color,
                                                            l_color, l_color };

                    l_textBackground.layer = ( 1 );
                }

                // Text
                {
                    const uint16_t l_layer = ( 2 );
                    const uint8_t l_alpha = 0xFF;
                    const uint16_t l_shade = 0xFF;
                    const uint16_t l_shade2 = 0xFF;
                    const uint8_t l_fontSize = 13;
                    const uint8_t l_textBackgroundLeftRightPadding = 15;
                    const uint8_t l_textBackgroundTopBottomPadding = 7;
                    const int32_t l_leftMargin =
                        ( 50 + l_textBackgroundLeftRightPadding );
                    const uint8_t l_rowTopMargin =
                        ( 6 + l_textBackgroundTopBottomPadding );
                    const uint8_t l_maxItemsPerPage = 15;
                    const uint16_t l_firstIndexToShow =
                        ( g_menuCursorIndex > ( l_maxItemsPerPage - 1 ) )
                            ? ( g_menuCursorIndex - ( l_maxItemsPerPage - 1 ) )
                            : ( 0 );
                    uint16_t l_index = 0;
                    uint16_t l_indexForShown = 0;

                    for ( const auto& _item : g_jsonControlsKeyboard.items() ) {
                        if ( l_index < l_firstIndexToShow ) {
                            l_index++;

                            continue;
                        }

                        if ( l_indexForShown >= l_maxItemsPerPage ) {
                            break;
                        }

                        const int32_t l_rowY =
                            ( 50 + l_overlayY +
                              ( int32_t )( ( l_indexForShown * l_fontSize ) +
                                           ( l_indexForShown *
                                             l_rowTopMargin ) ) );
                        std::string l_valueContent =
                            _item.value().template get< std::string >();

                        const uint32_t l_maxValueWidth = 325;

                        if ( l_valueContent.length() >
                             ( l_maxValueWidth / l_fontSize ) ) {
                            l_valueContent.resize(
                                ( l_maxValueWidth / l_fontSize ) - 3 );
                            std::fill_n( l_valueContent.rbegin(), 3, '.' );
                        }

                        const uint32_t l_valueWidth =
                            ( l_valueContent.length() * l_fontSize );

                        std::string l_keyContent = "";

                        if ( g_keyboardLayout.find( std::stoi(
                                 _item.key() ) ) != g_keyboardLayout.end() ) {
                            l_keyContent = ( g_keyboardLayout.at(
                                std::stoi( _item.key() ) ) );
                        }

                        const uint32_t l_keyWidth =
                            ( l_keyContent.length() * l_fontSize );
                        const int32_t l_keyX =
                            ( *( int32_t* )SCREEN_WIDTH - l_keyWidth -
                              ( l_leftMargin * 2 ) );

                        l_texts.push_back( { l_layer,
                                             l_alpha,
                                             l_shade,
                                             l_shade2,
                                             { l_leftMargin, l_rowY },
                                             { l_fontSize, l_fontSize },
                                             l_valueContent } );

                        l_textBackground.size = {
                            ( l_valueWidth +
                              ( l_textBackgroundLeftRightPadding * 2 ) ),
                            ( l_fontSize +
                              ( l_textBackgroundTopBottomPadding * 2 ) ) };
                        l_textBackground.coordinates = {
                            ( l_leftMargin - l_textBackgroundLeftRightPadding ),
                            ( l_rowY - l_textBackgroundTopBottomPadding ) };

                        if ( l_indexForShown ==
                             ( g_menuCursorIndex - l_firstIndexToShow ) ) {
                            struct rectangle l_textBackgroundSelected =
                                l_textBackground;

                            const uint8_t l_alpha = 0xFF;
                            const uint8_t l_red = 0xFF;
                            const uint8_t l_green = 0;
                            const uint8_t l_blue = 0;
                            uint32_t l_color = 0;

                            _useCallback( "native$getColorForRectangle",
                                          &l_alpha, &l_red, &l_green, &l_blue,
                                          &l_color );

                            l_textBackgroundSelected.colorsForRectangle = {
                                l_color, l_color, l_color, l_color };

                            l_rectangles.push_back( l_textBackgroundSelected );

                        } else {
                            l_rectangles.push_back( l_textBackground );
                        }

                        l_texts.push_back( { l_layer,
                                             l_alpha,
                                             l_shade,
                                             l_shade2,
                                             { l_keyX, l_rowY },
                                             { l_fontSize, l_fontSize },
                                             l_keyContent } );

                        l_textBackground.size = {
                            ( l_keyWidth +
                              ( l_textBackgroundLeftRightPadding * 2 ) ),
                            ( l_fontSize +
                              ( l_textBackgroundTopBottomPadding * 2 ) ) };
                        l_textBackground.coordinates = {
                            ( l_keyX - l_textBackgroundLeftRightPadding ),
                            ( l_rowY - l_textBackgroundTopBottomPadding ) };

                        if ( l_indexForShown ==
                             ( g_menuCursorIndex - l_firstIndexToShow ) ) {
                            struct rectangle l_textBackgroundSelected =
                                l_textBackground;

                            const uint8_t l_alpha = 0xFF;
                            const uint8_t l_red = 0;
                            const uint8_t l_green = static_cast< uint8_t >(
                                0x59 * ( g_activeFlagsOverlay &
                                         OVERLAY_IS_MAPPING_KEY ) );
                            const uint8_t l_blue = 0xFF;
                            uint32_t l_color = 0;

                            _useCallback( "native$getColorForRectangle",
                                          &l_alpha, &l_red, &l_green, &l_blue,
                                          &l_color );

                            l_textBackgroundSelected.colorsForRectangle = {
                                l_color, l_color, l_color, l_color };

                            l_rectangles.push_back( l_textBackgroundSelected );

                        } else {
                            l_rectangles.push_back( l_textBackground );
                        }

                        l_index++;
                        l_indexForShown++;
                    }
                }
            }
        }

        // Render
        {
            // Background
            {
                for ( const struct rectangle _rectangle : l_rectangles ) {
                    _useCallback(
                        "native$drawRectangle", &_rectangle.coordinates.x,
                        &_rectangle.coordinates.y, &_rectangle.size.width,
                        &_rectangle.size.height,
                        &_rectangle.colorsForRectangle.a,
                        &_rectangle.colorsForRectangle.b,
                        &_rectangle.colorsForRectangle.c,
                        &_rectangle.colorsForRectangle.d, &_rectangle.layer );
                }
            }

            // UI
            {
                // Text
                {
                    uintptr_t* l_fontAddress = ( uintptr_t* )FONT2;
                    uint32_t l_letterSpacing = 0;
                    char* l_out = 0;

                    for ( const struct text _text : l_texts ) {
                        _useCallback( "native$drawText", &_text.size.width,
                                      &_text.size.height, &_text.coordinates.x,
                                      &_text.coordinates.y, &_text.content,
                                      &_text.alpha, &_text.shade, &_text.shade2,
                                      l_fontAddress, &l_letterSpacing,
                                      &_text.layer, &l_out );
                    }
                }
            }
        }

    } else {
        l_animationNeeded = true;
    }

    return ( 0 );
}
#endif
