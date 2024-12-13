#include <d3d9.h>
#include <stdio.h>

#include "_useCallback.h"
#include "direction_t.h"
#include "native.h"
#include "overlay.h"
#include "stdfunc.h"

useCallbackFunction_t g_useCallback;
element_t** g_overlayToRender = NULL;
static size_t g_frameCounter = 0;

static uint16_t needToMove( const char** _activeMappedKeys ) {
    uint16_t l_returnValue = 0;

    if ( g_frameCounter ) {
        goto EXIT;
    }

    if ( _containsString( _activeMappedKeys, "2" ) ) {
        l_returnValue = DOWN;

    } else if ( _containsString( _activeMappedKeys, "6" ) ) {
        l_returnValue = RIGHT;

    } else if ( _containsString( _activeMappedKeys, "8" ) ) {
        l_returnValue = UP;

    } else if ( _containsString( _activeMappedKeys, "4" ) ) {
        l_returnValue = LEFT;
    }

    if ( l_returnValue ) {
        g_frameCounter = 10;
    }

EXIT:
    return ( l_returnValue );
}

static uint16_t bind$needToMove( const char** _activeMappedKeys ) {
    uint16_t l_returnValue = 0;

    l_returnValue = needToMove( _activeMappedKeys );

    return ( l_returnValue );
}

uint16_t __declspec( dllexport ) IDirect3D9Ex$CreateDevice(
    void** _callbackArguments ) {
    _useCallbackInitialize();

    g_overlaysToRender = ( element_t*** )createArray( sizeof( element_t** ) );
    g_overlayHotkeys = ( char** )createArray( sizeof( char* ) );
    g_overlayNames = ( char** )createArray( sizeof( char* ) );

    return ( 0 );
}

uint16_t __declspec( dllexport ) mainLoop$end( void** _callbackArguments ) {
    uint16_t l_returnValue = 0;

    if ( g_frameCounter ) {
        g_frameCounter--;
    }

    return ( l_returnValue );
}

uint16_t __declspec( dllexport ) keyboard$getInput$end(
    void** _callbackArguments ) {
    uint16_t l_returnValue = 0;

    const char*** _activeMappedKeys = ( const char*** )_callbackArguments[ 0 ];
    const char*** _activeKeys = ( const char*** )_callbackArguments[ 1 ];

    static bool l_isOverlayInteractive = false;

    if ( g_frameCounter ) {
        goto INTERACT_ELEMENTS;
    }

    if ( !arrayLength( *_activeMappedKeys ) ) {
        if ( !arrayLength( *_activeKeys ) ) {
            goto EXIT;

        } else {
            goto INTERACT_ELEMENTS;
        }
    }

    FOR_ARRAY( char* const*, g_overlayHotkeys ) {
        if ( _containsString( *_activeMappedKeys, *_element ) ) {
            _useCallback( "log$transaction$query", "Overlay hotkey " );
            _useCallback( "log$transaction$query", *_element );
            _useCallback( "log$transaction$query", "is active\n" );

            if ( g_overlayToRender == NULL ) {
                g_overlayToRender = arrayFirstElementPointer(
                    g_overlaysToRender )[ _element - arrayFirstElementPointer(
                                                         g_overlayHotkeys ) ];

                {
                    l_isOverlayInteractive = false;

                    FOR_ARRAY( element_t**, g_overlayToRender ) {
                        if ( ( *_element )->canActive ) {
                            l_isOverlayInteractive = true;
                        }
                    }
                }

            } else {
                g_overlayToRender = NULL;
            }

            g_frameCounter = 30;

            goto EXIT;
        }
    }

INTERACT_ELEMENTS:
    if ( g_overlayToRender != NULL ) {
        FOR_ARRAY( element_t**, g_overlayToRender ) {
            if ( ( *_element )->isActive ) {
                const direction_t l_interactionReturnValue =
                    ( direction_t )( interactElement(
                        *_element, _activeMappedKeys, _activeKeys ) );

                if ( ( l_interactionReturnValue ) &&
                     ( l_interactionReturnValue != ENODATA ) ) {
                    // Next
                    if ( ( l_interactionReturnValue == DOWN ) ||
                         ( l_interactionReturnValue == RIGHT ) ) {
                        bool l_isNextToActivate = true;

                        ( *_element )->isActive = false;

                        // Move current to next
                        _element++;

                        // Forward from current
                        while ( _element !=
                                ( arrayLastElementPointer( g_overlayToRender ) +
                                  1 ) ) {
                            if ( ( *_element )->canActive ) {
                                ( *_element )->isActive = true;
                                l_isNextToActivate = false;

                                break;
                            }

                            _element++;
                        }

                        if ( l_isNextToActivate ) {
                            // Forward from the beginning
                            FOR_ARRAY( element_t**, g_overlayToRender ) {
                                if ( ( *_element )->canActive ) {
                                    ( *_element )->isActive = true;

                                    break;
                                }
                            }
                        }

                        break;

                        // Previous
                    } else if ( ( l_interactionReturnValue == UP ) ||
                                ( l_interactionReturnValue == LEFT ) ) {
                        bool l_isPreviousToActivate = true;

                        ( *_element )->isActive = false;

                        // Move current to previous
                        _element--;

                        // Backward from current
                        while ( _element != ( arrayFirstElementPointer(
                                                  g_overlayToRender ) -
                                              1 ) ) {
                            if ( ( *_element )->canActive ) {
                                ( *_element )->isActive = true;
                                l_isPreviousToActivate = false;

                                break;
                            }

                            _element--;
                        }

                        if ( l_isPreviousToActivate ) {
                            _element =
                                arrayLastElementPointer( g_overlayToRender );

                            // Backward from the end
                            while ( _element != ( arrayFirstElementPointer(
                                                      g_overlayToRender ) -
                                                  1 ) ) {
                                if ( ( *_element )->canActive ) {
                                    ( *_element )->isActive = true;
                                    l_isPreviousToActivate = false;

                                    break;
                                }

                                _element--;
                            }
                        }

                        break;
                    }
                }
            }
        }

        {
            free( *_activeMappedKeys );

            *_activeMappedKeys = ( const char** )createArray( sizeof( char* ) );

            free( *_activeKeys );

            *_activeKeys = ( const char** )createArray( sizeof( char* ) );
        }
    }

EXIT:
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
    const element_t* _element = ( const element_t* )_callbackArguments[ 0 ];

    const uint32_t l_a = getColorForRectangle( _element->a );
    const uint32_t l_b = getColorForRectangle( _element->b );
    const uint32_t l_c = getColorForRectangle( _element->c );
    const uint32_t l_d = getColorForRectangle( _element->d );

#if 0
    printf( "rectangle x%u y%u w%u h%u a%u b%u c%u d%u l%u\n",
            _element->coordinates.x, _element->coordinates.y,
            _element->size.width, _element->size.height, l_a, l_b, l_c, l_d,
            _element->layer );
#endif

    drawRectangle( _element->coordinates.x, _element->coordinates.y,
                   _element->size.width, _element->size.height, l_a, l_b, l_c,
                   l_d, _element->layer );

    return ( l_returnValue );
}

uint16_t __declspec( dllexport ) overlay$draw$text(
    void** _callbackArguments ) {
    uint16_t l_returnValue = 0;
    const element_t* _element = ( const element_t* )_callbackArguments[ 0 ];
    char* l_out;

#if 0
    printf( "text %u %u %d %d %s %u %u %u %u %u %u %p\n", _element->size.width,
            _element->size.height, _element->coordinates.x,
            _element->coordinates.y, _element->text, _element->a.alpha,
            _element->shade.first, _element->shade.second,
            _element->fontAddress, _element->letterSpacing, _element->layer,
            l_out );
#endif

    drawText( _element->size.width, _element->size.height,
              _element->coordinates.x, _element->coordinates.y, _element->text,
              _element->a.alpha, _element->shade.first, _element->shade.second,
              _element->fontAddress, _element->letterSpacing, _element->layer,
              l_out );

    return ( l_returnValue );
}

uint16_t __declspec( dllexport ) overlay$interact$bind$binding$needToMove(
    void** _callbackArguments ) {
    uint16_t l_returnValue = 0;
    const element_t* _element = ( const element_t* )_callbackArguments[ 0 ];
    const char*** _activeMappedKeys = ( const char*** )_callbackArguments[ 1 ];

    l_returnValue = bind$needToMove( *_activeMappedKeys );

    return ( l_returnValue );
}

static bool inline bind$activate( element_t* _element,
                                  color_t* _lastElementColor ) {
    bool l_returnValue = false;

    _useCallback( "log$transaction$query", "Bind activated\n" );

    _lastElementColor->red = _element->a.red;
    _lastElementColor->green = _element->a.green;
    _lastElementColor->blue = _element->a.blue;

    if ( _lastElementColor->red < 125 ) {
        _element->a.red -= 60;

    } else {
        _element->a.red += 60;
    }

    if ( _lastElementColor->green < 125 ) {
        _element->a.green -= 60;

    } else {
        _element->a.green += 60;
    }

    if ( _lastElementColor->blue < 125 ) {
        _element->a.blue -= 60;

    } else {
        _element->a.blue += 60;
    }

    l_returnValue = true;

    return ( l_returnValue );
}

static bool inline bind$deactivate( element_t* _element,
                                    color_t _lastElementColor ) {
    bool l_returnValue = false;

    _useCallback( "log$transaction$query", "Bind deactivated\n" );

    _element->a.red = _lastElementColor.red;
    _element->a.green = _lastElementColor.green;
    _element->a.blue = _lastElementColor.blue;

    return ( l_returnValue );
}

uint16_t __declspec( dllexport ) overlay$interact$bind(
    void** _callbackArguments ) {
    uint16_t l_returnValue = 0;
    element_t* _element = ( element_t* )_callbackArguments[ 0 ];
    const char*** _activeMappedKeys = ( const char*** )_callbackArguments[ 1 ];
    const char*** _activeKeys = ( const char*** )_callbackArguments[ 2 ];

    static bool l_isBinding = false;
    static color_t l_lastElementColor;

    if ( !l_isBinding ) {
        if ( _containsString( *_activeMappedKeys, "A" ) ) {
            l_isBinding = bind$activate( _element, &l_lastElementColor );
        }
    }

    if ( l_isBinding ) {
        if ( _containsString( *_activeMappedKeys, "B" ) ) {
            l_isBinding = bind$deactivate( _element, l_lastElementColor );
        }
    }

    if ( !l_isBinding ) {
        l_returnValue =
            _useCallback( "overlay$interact$bind$binding$needToMove", _element,
                          _activeMappedKeys );
    }

    if ( l_isBinding ) {
        if ( !arrayLength( *_activeKeys ) ) {
            goto BIND_EXIT;
        }

        const char* l_valueToBind =
            *( arrayFirstElementPointer( *_activeKeys ) );

        if ( strcmp( l_valueToBind, _element->text ) == 0 ) {
            goto BIND_EXIT;
        }

        if ( _element->callbackAddress ) {
            const char** l_callbackValuePointer =
                ( const char** )( _element->callbackAddress );

            *l_callbackValuePointer = l_valueToBind;
        }

        _useCallback( "log$transaction$query", _element->text );
        _useCallback( "log$transaction$query", "\n" );

        l_returnValue =
            _useCallback( "overlay$interact$bind$binding$end", _element );

        _element->text = ( char* )l_valueToBind;
        _useCallback( "log$transaction$query", _element->text );
        _useCallback( "log$transaction$query", "\n" );

        l_isBinding = bind$deactivate( _element, l_lastElementColor );
    BIND_EXIT:
    }

    return ( l_returnValue );
}

uint16_t __declspec( dllexport ) overlay$draw$bind(
    void** _callbackArguments ) {
    uint16_t l_returnValue = 0;
    const element_t* _element = ( const element_t* )_callbackArguments[ 0 ];

    const size_t l_textLength = strlen( _element->text );
    const size_t l_fontSize =
        ( l_textLength ) ? ( _element->size.width / l_textLength ) : ( 12 );

    const coordinates_t l_fontCoordinates = { ( _element->coordinates.x ),
                                              ( _element->coordinates.y ) };

    color_t l_tempA = _element->a;

    if ( _element->isActive ) {
        l_tempA.red = 255;
        l_tempA.green = 0;
    }

    const uint32_t l_a = getColorForRectangle( l_tempA );
    const uint32_t l_b = getColorForRectangle( _element->b );
    const uint32_t l_c = getColorForRectangle( _element->c );
    const uint32_t l_d = getColorForRectangle( _element->d );

    drawRectangle( _element->coordinates.x, _element->coordinates.y,
                   _element->size.width, _element->size.height, l_a, l_b, l_c,
                   l_d, _element->layer );

    char* l_out;

    drawText( l_fontSize, l_fontSize, l_fontCoordinates.x, l_fontCoordinates.y,
              _element->text, _element->a.alpha, _element->shade.first,
              _element->shade.second, _element->fontAddress,
              _element->letterSpacing, _element->layer, l_out );

    return ( l_returnValue );
}
