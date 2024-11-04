#include <d3d9.h>
#include <stdio.h>

#include "_useCallback.h"
#include "native.h"
#include "overlay.h"
#include "stdfunc.h"

useCallbackFunction_t g_useCallback;
element_t** g_overlayToRender = NULL;

IDirect3DDevice9*** g_directXDevice;

uint16_t __declspec( dllexport ) IDirect3D9Ex$CreateDevice(
    void** _callbackArguments ) {
    _useCallbackInitialize();

    g_overlaysToRender = ( element_t*** )createArray( sizeof( element_t** ) );
    g_overlayHotkeys = ( char** )createArray( sizeof( char* ) );
    g_overlayNames = ( char** )createArray( sizeof( char* ) );

    g_directXDevice = ( IDirect3DDevice9*** )_callbackArguments[ 5 ];

    return ( 0 );
}

uint16_t __declspec( dllexport ) keyboard$getInput$end(
    void** _callbackArguments ) {
    uint16_t l_returnValue = 0;

    static size_t l_frameCounter = 0;

    if ( l_frameCounter ) {
        goto EXIT;
    }

    char*** _activeMappedKeys = ( char*** )_callbackArguments[ 0 ];
    char*** _activeKeys = ( char*** )_callbackArguments[ 1 ];

    if ( !arrayLength( *_activeMappedKeys ) ) {
        goto NOT_MAPPED;
    }

    FOR_ARRAY( char* const*, g_overlayHotkeys ) {
        if ( _containsString( *_activeMappedKeys, *_element ) ) {
            printf( "KEY TRUE  %s\n", *_element );

            if ( g_overlayToRender == NULL ) {
                g_overlayToRender = arrayFirstElementPointer(
                    g_overlaysToRender )[ _element - arrayFirstElementPointer(
                                                         g_overlayHotkeys ) ];

                printf(
                    "%s\n",
                    arrayFirstElementPointer( g_overlayToRender )[ 0 ]->text );

            } else {
                g_overlayToRender = NULL;
            }

            l_frameCounter = 30;

            goto EXIT;
        }
    }

NOT_MAPPED:
    if ( !arrayLength( *_activeKeys ) ) {
        goto EXIT;
    }

EXIT:
    if ( l_frameCounter ) {
        l_frameCounter--;
    }

    return ( l_returnValue );
}

uint16_t __declspec( dllexport ) game$frame$extraDraw(
    void** _callbackArguments ) {
    uint16_t l_returnValue = 0;

    if ( g_overlayToRender != NULL ) {
        FOR_ARRAY( element_t**, g_overlayToRender ) {
            drawElement( *_element );
        }
    }

    return ( l_returnValue );
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
        char** l_elementsOrder;

        {
            char* l_elementsOrderString = getKeyFromSettingsOrDefault(
                _overlayName, "overlay_items_order", _elementsDefaultOrder );

            const char l_delimiter[] = ",";
            l_elementsOrder =
                splitStringIntoArray( l_elementsOrderString, l_delimiter );

            free( l_elementsOrderString );
        }

        if ( arrayLength( l_elementsOrder ) ) {
            l_returnValue = overlayRegister(
                _overlayName, ( const char* const* )l_elementsOrder,
                _elementsDefaultSettings, _elementsCallbackVariableReferences,
                _overlayDefaultHotkey );
        }

        FREE_ARRAY( char**, l_elementsOrder, *_element );
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

uint16_t __declspec( dllexport ) overlay$draw$rectangle(
    void** _callbackArguments ) {
    uint16_t l_returnValue = 0;
    const element_t* l_element = ( const element_t* )_callbackArguments[ 0 ];

    const uint32_t l_a = getColorForRectangle( l_element->a );
    const uint32_t l_b = getColorForRectangle( l_element->b );
    const uint32_t l_c = getColorForRectangle( l_element->c );
    const uint32_t l_d = getColorForRectangle( l_element->d );

#if 0
    printf( "rectangle x%u y%u w%u h%u a%u b%u c%u d%u l%u\n",
            l_element->coordinates.x, l_element->coordinates.y,
            l_element->size.width, l_element->size.height, l_a, l_b, l_c, l_d,
            l_element->layer );
#endif

    drawRectangle( l_element->coordinates.x, l_element->coordinates.y,
                   l_element->size.width, l_element->size.height, l_a, l_b, l_c,
                   l_d, l_element->layer );

    return ( l_returnValue );
}

uint16_t __declspec( dllexport ) overlay$draw$text(
    void** _callbackArguments ) {
    uint16_t l_returnValue = 0;
    const element_t* l_element = ( const element_t* )_callbackArguments[ 0 ];
    char* l_out;

#if 0
    printf( "text %u %u %d %d %s %u %u %u %u %u %u %p\n", l_element->size.width,
            l_element->size.height, l_element->coordinates.x,
            l_element->coordinates.y, l_element->text, l_element->a.alpha,
            l_element->shade.first, l_element->shade.second,
            l_element->fontAddress, l_element->letterSpacing, l_element->layer,
            l_out );
#endif

    drawText( l_element->size.width, l_element->size.height,
              l_element->coordinates.x, l_element->coordinates.y,
              l_element->text, l_element->a.alpha, l_element->shade.first,
              l_element->shade.second, l_element->fontAddress,
              l_element->letterSpacing, l_element->layer, l_out );

    return ( l_returnValue );
}
