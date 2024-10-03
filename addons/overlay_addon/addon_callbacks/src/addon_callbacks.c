#include <stdio.h>

#include "_useCallback.h"
#include "native.h"
#include "overlay.h"
#include "stdfunc.h"

useCallbackFunction_t g_useCallback;

uint16_t __declspec( dllexport ) IDirect3D9Ex$CreateDevice(
    void** _callbackArguments ) {
    _useCallbackInitialize();

    g_elementsToRender = ( element_t** )createArray( sizeof( element_t* ) );
    g_overlayHotkeys = ( char** )createArray( sizeof( char* ) );
    g_overlayNames = ( char** )createArray( sizeof( char* ) );

    return ( 0 );
}

/*
 * char*      overlayName
 * char*      elementsDefaultOrder
 * char*      elementsDefaultSettings
 * uintptr_t* elementsCallbackVariableReferences
 * char*      overlayDefaultHotkey
 */
uint16_t __declspec( dllexport ) overlay$register( void** _callbackArguments ) {
    uint16_t l_returnValue = 0;
    const char* _overlayName;
    const char* _elementsDefaultOrder;
    const char* _elementsDefaultSettings;
    const uintptr_t* _elementsCallbackVariableReferences;
    const char* _overlayDefaultHotkey;

    _useCallback( "log$transaction$query", "Starting to register overlay\n" );

    // Get arguments
    {
        _overlayName = ( const char* )_callbackArguments[ 0 ];
        {
            _useCallback( "log$transaction$query", "Overlay name : \"" );
            _useCallback( "log$transaction$query", _overlayName );
            _useCallback( "log$transaction$query", "\"\n" );
        }

        _elementsDefaultOrder = ( char* )_callbackArguments[ 1 ];
        {
            _useCallback( "log$transaction$query",
                          "Elements default order : {\n" );
            _useCallback( "log$transaction$query", _elementsDefaultOrder );
            _useCallback( "log$transaction$query", "\n}\n" );
        }

        _elementsDefaultSettings = ( char* )_callbackArguments[ 2 ];
        {
            _useCallback( "log$transaction$query",
                          "Elements default settings : {\n" );
            _useCallback( "log$transaction$query", _elementsDefaultSettings );
            _useCallback( "log$transaction$query", "\n}\n" );
        }

        _elementsCallbackVariableReferences =
            ( uintptr_t* )_callbackArguments[ 3 ];
        {
            char* l_elementsCallbackVariableReferencesAsText =
                stoa( ( size_t )_elementsCallbackVariableReferences );

            _useCallback( "log$transaction$query",
                          "Elements callback variables references : \"" );
            _useCallback( "log$transaction$query",
                          l_elementsCallbackVariableReferencesAsText );
            _useCallback( "log$transaction$query", "\"\n" );

            free( l_elementsCallbackVariableReferencesAsText );
        }

        _overlayDefaultHotkey = ( char* )_callbackArguments[ 4 ];
        {
            _useCallback( "log$transaction$query",
                          "Overlay default hotkey : \"" );
            _useCallback( "log$transaction$query", _overlayDefaultHotkey );
            _useCallback( "log$transaction$query", "\"\n" );
        }
    }

    {
        const char** l_elementsOrder =
            ( const char** )createArray( sizeof( const char* ) );
        const char* l_elementsOrderString;

        {
            char*** l_settings;

            if ( ( l_returnValue =
                       _useCallback( "core$getSettingsContentByLabel",
                                     &l_settings, _overlayName ) ) == 0 ) {
                const ssize_t l_elementsOrderIndex =
                    findKeyInSettings( l_settings, "overlay_items_order" );

                if ( l_elementsOrderIndex >= 0 ) {
                    l_elementsOrderString =
                        l_settings[ l_elementsOrderIndex ][ 1 ];

                } else {
                    _useCallback( "core$changeSettingsKeyByLabel",
                                  "overlay_items_order", _overlayName,
                                  _elementsDefaultOrder );

                    l_elementsOrderString = _elementsDefaultOrder;
                }
            }
        }

        {
            char* l_text = strdup( l_elementsOrderString );
            const char l_delimiter[] = ",";
            char* l_splitted = strtok( l_text, l_delimiter );

            while ( l_splitted ) {
                insertIntoArray( ( void*** )&l_elementsOrder,
                                 strdup( l_splitted ) );

                l_splitted = strtok( NULL, l_delimiter );
            }

            free( l_text );
        }

        if ( arrayLength( l_elementsOrder ) ) {
            l_returnValue = overlayRegister(
                _overlayName, ( const char* const* )l_elementsOrder,
                _elementsDefaultSettings, _elementsCallbackVariableReferences,
                _overlayDefaultHotkey );
        }

        {
            const char** l_content = l_elementsOrder;
            const size_t l_contentLength = arrayLength( l_content );
            const char** l_contentFirstElement =
                arrayFirstElementPointer( l_content );
            const char* const* l_contentEnd =
                ( l_contentFirstElement + l_contentLength );

            for ( const char** _item = l_contentFirstElement;
                  _item != l_contentEnd; _item++ ) {
                free( _item );
            }
        }

        free( l_elementsOrder );
    }

    {
        char* l_returnValueAsText = stoa( l_returnValue );

        _useCallback( "log$transaction$query",
                      "Finished registering overlay with code : " );
        _useCallback( "log$transaction$query", l_returnValueAsText );
        _useCallback( "log$transaction$query", "\n" );

        free( l_returnValueAsText );
    }

    return ( l_returnValue );
}

uint16_t __declspec( dllexport ) keyboard$getInput$end(
    void** _callbackArguments ) {
    uint16_t l_returnValue = 0;

    return ( l_returnValue );
}

#if 0
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
