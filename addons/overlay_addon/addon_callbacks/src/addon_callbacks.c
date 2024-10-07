#include <stdio.h>

#include "_useCallback.h"
#include "native.h"
#include "overlay.h"
#include "stdfunc.h"

useCallbackFunction_t g_useCallback;
element_t** g_overlayToRender = NULL;

uint16_t __declspec( dllexport ) IDirect3D9Ex$CreateDevice(
    void** _callbackArguments ) {
    _useCallbackInitialize();

    g_overlaysToRender = ( element_t*** )createArray( sizeof( element_t** ) );
    g_overlayHotkeys = ( char** )createArray( sizeof( char* ) );
    g_overlayNames = ( char** )createArray( sizeof( char* ) );

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
        printf( "TEST\n" );
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
        const char** l_elementsOrder =
            ( const char** )createArray( sizeof( const char* ) );
        char* l_elementsOrderString;

        {
            char*** l_settings;

            if ( ( l_returnValue =
                       _useCallback( "core$getSettingsContentByLabel",
                                     &l_settings, _overlayName ) ) == 0 ) {
                const ssize_t l_elementsOrderIndex =
                    findKeyInSettings( l_settings, "overlay_items_order" );

                if ( l_elementsOrderIndex >= 0 ) {
                    l_elementsOrderString =
                        strdup( l_settings[ l_elementsOrderIndex ][ 1 ] );

                    freeSettingsContent( l_settings );

                } else {
                    _useCallback( "core$changeSettingsKeyByLabel",
                                  "overlay_items_order", _overlayName,
                                  _elementsDefaultOrder );

                    l_elementsOrderString = strdup( _elementsDefaultOrder );
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

            FOR_ARRAY( const char**, l_elementsOrder ) {
                free( _element );
            }
        }

        free( l_elementsOrder );
        free( l_elementsOrderString );
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
