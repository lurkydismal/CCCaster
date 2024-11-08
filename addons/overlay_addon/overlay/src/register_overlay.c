#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "_useCallback.h"
#include "native.h"
#include "overlay.h"
#include "stdfunc.h"

static inline void setElementPropertyByKey( element_t* _element,
                                            const char* _key,
                                            char* _value ) {
    const size_t l_valueAsSize = atol( _value );

    if ( strcmp( _key, "x" ) == 0 ) {
        _element->coordinates.x = l_valueAsSize;

    } else if ( strcmp( _key, "y" ) == 0 ) {
        _element->coordinates.y = l_valueAsSize;

    } else if ( strcmp( _key, "texture_x" ) == 0 ) {
        _element->textureCoordinates.x = l_valueAsSize;

    } else if ( strcmp( _key, "texture_y" ) == 0 ) {
        _element->textureCoordinates.y = l_valueAsSize;

    } else if ( strcmp( _key, "width" ) == 0 ) {
        _element->size.width = l_valueAsSize;

    } else if ( strcmp( _key, "height" ) == 0 ) {
        _element->size.height = l_valueAsSize;

    } else if ( strcmp( _key, "texture_width" ) == 0 ) {
        _element->textureSize.width = l_valueAsSize;

    } else if ( strcmp( _key, "texture_height" ) == 0 ) {
        _element->textureSize.height = l_valueAsSize;

    } else if ( strcmp( _key, "red" ) == 0 ) {
        const uint8_t l_red = l_valueAsSize;

        _element->a.red = l_red;

        if ( _element->type == RECTANGLE ) {
            _element->b.red = l_red;
            _element->c.red = l_red;
            _element->d.red = l_red;
        }

    } else if ( strcmp( _key, "green" ) == 0 ) {
        const uint8_t l_green = l_valueAsSize;

        _element->a.green = l_green;

        if ( _element->type == RECTANGLE ) {
            _element->b.green = l_green;
            _element->c.green = l_green;
            _element->d.green = l_green;
        }

    } else if ( strcmp( _key, "blue" ) == 0 ) {
        const uint8_t l_blue = l_valueAsSize;

        _element->a.blue = l_blue;

        if ( _element->type == RECTANGLE ) {
            _element->b.blue = l_blue;
            _element->c.blue = l_blue;
            _element->d.blue = l_blue;
        }

    } else if ( strcmp( _key, "alpha" ) == 0 ) {
        const uint8_t l_alpha = l_valueAsSize;

        _element->a.alpha = l_alpha;

        if ( _element->type == RECTANGLE ) {
            _element->b.alpha = l_alpha;
            _element->c.alpha = l_alpha;
            _element->d.alpha = l_alpha;
        }

    } else if ( strcmp( _key, "text" ) == 0 ) {
        _element->text = _value;

    } else if ( strcmp( _key, "shade_first" ) == 0 ) {
        _element->shade.first = l_valueAsSize;

    } else if ( strcmp( _key, "shade_second" ) == 0 ) {
        _element->shade.second = l_valueAsSize;

    } else if ( strcmp( _key, "letter_spacing" ) == 0 ) {
        _element->letterSpacing = l_valueAsSize;

    } else if ( strcmp( _key, "layer" ) == 0 ) {
        _element->layer = l_valueAsSize;

    } else {
        // Color
        if ( _key[ 1 ] == '_' ) {
            color_t* l_elementColor;

            switch ( _key[ 0 ] ) {
                case 'a': {
                    l_elementColor = &_element->a;

                    break;
                }

                case 'b': {
                    l_elementColor = &_element->b;

                    break;
                }

                case 'c': {
                    l_elementColor = &_element->c;

                    break;
                }

                case 'd': {
                    l_elementColor = &_element->d;

                    break;
                }

                default: {
                    return;
                }
            }

            const char* l_color = ( _key + 2 );

            if ( strcmp( l_color, "red" ) == 0 ) {
                l_elementColor->red = l_valueAsSize;

            } else if ( strcmp( l_color, "green" ) == 0 ) {
                l_elementColor->green = l_valueAsSize;

            } else if ( strcmp( l_color, "blue" ) == 0 ) {
                l_elementColor->blue = l_valueAsSize;

            } else if ( strcmp( l_color, "alpha" ) == 0 ) {
                l_elementColor->alpha = l_valueAsSize;
            }
        }
    }
}

static inline enum elementType getElementTypeFromLabel(
    const char* _elementLabel ) {
    printf( "getElementTypeFromLabel %s\n", _elementLabel );
    FOR( char* const*, g_elementTypesAsString ) {
        if ( strcmp( *_element, _elementLabel ) == 0 ) {
            printf( "LT %s %d\n", _elementLabel,
                    ( _element - g_elementTypesAsString ) );
            return ( _element - g_elementTypesAsString );
        }
    }
}

static inline const char* getElementLabelFromType(
    const enum elementType _elementType ) {
    printf( "getElementLabelFromType %d\n", _elementType );
    printf( "LT %d %s\n", _elementType,
            g_elementTypesAsString[ _elementType ] );

    return ( g_elementTypesAsString[ _elementType ] );
}

static inline ssize_t increaseElementCount( char*** _elementLabels,
                                            size_t** _countsArray,
                                            const char* _label ) {
    const ssize_t l_labelIndex = _findStringInArray( *_elementLabels, _label );

    if ( l_labelIndex >= 1 ) {
        ( *_countsArray[ l_labelIndex ] )++;

    } else {
        insertIntoArray( ( void*** )_elementLabels,
                         ( void* )( strdup( _label ) ) );
        insertIntoArray( ( void*** )_countsArray, ( void* )1 );
    }

    return ( l_labelIndex );
}

static inline size_t getElementCount( char** _elementLabels,
                                      size_t* _countsArray,
                                      const char* _label ) {
    ssize_t l_returnValue = -1;
    const ssize_t l_labelIndex = _findStringInArray( _elementLabels, _label );

    if ( l_labelIndex >= 1 ) {
        l_returnValue = _countsArray[ l_labelIndex ];
    }

    return ( l_labelIndex );
}

static inline char* mangleElementLabel( const char* _label,
                                        const char* _overlayName,
                                        const char* _labelIndexAsText ) {
    char* l_returnValue = strdup( "_" );

    concatBeforeAndAfterString( &l_returnValue, _overlayName, _label );
    concatBeforeAndAfterString( &l_returnValue, "", _labelIndexAsText );

    return ( l_returnValue );
}

static inline element_t* createElementWithSettings(
    const enum elementType _type,
    char*** _settings ) {
    element_t* l_element = ( element_t* )malloc( sizeof( element_t ) );
    *l_element = ( element_t )DEFAULT_ELEMENT_PARAMETERS;
    l_element->type = _type;

    FOR_ARRAY( char** const*, _settings ) {
        char* l_key = ( *_element )[ 0 ];
        char* l_value = ( *_element )[ 1 ];

        setElementPropertyByKey( l_element, ( const char* )l_key, l_value );
    }

    return ( l_element );
}

// Fill resort settings for overlay, in case of settings not having these
// already
static uint16_t getElementsSettings( char*** _elementsLabels,
                                     char*** _elementsSettings,
                                     const char* _overlayName,
                                     const char* _elementsDefaultSettings ) {
    uint16_t l_returnValue = 0;

    size_t* l_labelCounts = ( size_t* )createArray( sizeof( size_t ) );
    char** l_labels = ( char** )createArray( sizeof( char* ) );

    {
        char* l_text = strdup( _elementsDefaultSettings );

        const char l_delimiter[] = "\n";
        char* l_line = strtok( l_text, l_delimiter );
        char* l_buffer = strdup( "" );
        size_t l_bufferLength = 0;
        bool l_isFirstPass = true;

        while ( l_line ) {
            char* l_trimmedText = sanitizeString( l_line );
            const size_t l_textLength = strlen( l_trimmedText );

            if ( !l_textLength ) {
                goto PARSE_EXIT;
            }

            // Label
            if ( l_trimmedText[ 0 ] == '[' ) {
                if ( !l_isFirstPass ) {
                    l_buffer =
                        ( char* )realloc( l_buffer, ( l_bufferLength + 1 ) );

                    l_buffer[ l_bufferLength ] = '\0';

                    insertIntoArray( ( void*** )_elementsSettings,
                                     strdup( l_buffer ) );
                    printf( "SETTINGS %s\n", l_buffer );

                    free( l_buffer );
                    l_buffer = strdup( "" );

                    l_bufferLength = 0;
                }

                {
                    l_trimmedText++;
                    l_trimmedText[ l_textLength - 1 - 1 ] = '\0';
                    printf( "TEST13\n" );

                    const ssize_t l_elementIndex = increaseElementCount(
                        &l_labels, &l_labelCounts, l_trimmedText );
                    printf( "TEST14 %d\n", l_elementIndex );
                    const size_t l_labelCount = getElementCount(
                        l_labels, l_labelCounts, l_trimmedText );
                    printf( "TEST15 %d\n", l_labelCount );

                    {
                        const size_t l_labelIndex = ( l_labelCount - 1 );
                        char* l_labelIndexAsText = stoa( l_labelIndex );
                        printf( "TEST11\n" );

                        {
                            char* l_label =
                                mangleElementLabel( l_trimmedText, _overlayName,
                                                    l_labelIndexAsText );

                            printf( "TEST10\n" );
                            insertIntoArray( ( void*** )_elementsLabels,
                                             strdup( "keyboard_text0" ) );
                            printf( "_elementLabels[ %d ]\n%d %s\n",
                                    l_labelIndex,
                                    arrayLength( *_elementsLabels ),
                                    ( *_elementsLabels )[ l_labelIndex + 1 ] );

                            concatBeforeAndAfterString( &l_label, "[", "]" );
                            printf( "LABEL %s\n", l_label );

                            l_bufferLength = concatBeforeAndAfterString(
                                &l_buffer, "", l_label );
                            printf( "LB1 %s %d\n", l_label, strlen( l_label ) );

                            free( l_label );
                        }

                        free( l_labelIndexAsText );
                    }
                }

                // Not Label
            } else {
                l_bufferLength =
                    concatBeforeAndAfterString( &l_buffer, "", l_trimmedText );
                printf( "LB2 %s\n", l_trimmedText );
            }

            l_buffer[ l_bufferLength ] = '\n';
            l_bufferLength++;

            l_isFirstPass = false;

        PARSE_EXIT:
            free( l_trimmedText );

            printf( "LINE %s\n", l_line );
            l_line = strtok( NULL, l_delimiter );
        }

        printf( "TEST3\n" );

        if ( l_bufferLength == 0 ) {
            free( l_buffer );

        } else {
            l_buffer = ( char* )realloc( l_buffer, ( l_bufferLength + 1 ) );

            l_buffer[ l_bufferLength ] = '\0';

            insertIntoArray( ( void*** )_elementsSettings, strdup( l_buffer ) );
            printf( "SETTINGS %s\n", l_buffer );

            free( l_buffer );

            l_bufferLength = 0;
        }

        free( l_text );
    }

    FREE_ARRAY( char* const*, l_labels, *_element );

    free( l_labelCounts );

    return ( l_returnValue );
}

static inline const char* getElementDefaultSettings(
    const char* const* _elementsLabels,
    const char* const* _elementsSettings,
    const char* _elementLabel ) {
    const char* l_returnValue = NULL;
    printf( "ELEM %s\n", _elementsLabels[ 1 ] );
    const size_t l_elementDefaultSettingsIndex =
        _findStringInArray( _elementsLabels, _elementLabel );

    if ( l_elementDefaultSettingsIndex >= 1 ) {
        l_returnValue = _elementsSettings[ l_elementDefaultSettingsIndex ];
    }

    printf( "LABE1 %d\n", l_elementDefaultSettingsIndex );
    printf( "LABE2 %s\n", _elementsSettings[ 1 ] );

    return ( l_returnValue );
}

static inline void insertElementIntoOverlay( element_t*** _overlay,
                                             const enum elementType _type,
                                             char*** _settings ) {
    element_t* l_element = createElementWithSettings( _type, _settings );

    printf( "RE EL %s\n", l_element->text );
    insertIntoArray( ( void*** )_overlay, ( void* )( l_element ) );

    free( _settings );
}

static uint16_t registerElementsForRender(
    const char* _overlayName,
    const char* const* _elementsLabels,
    const char* const* _elementsOrder,
    const char* const* _elementsSettings ) {
    uint16_t l_returnValue = 0;

    element_t** l_overlay = ( element_t** )createArray( sizeof( element_t* ) );
    size_t* l_labelCounts = ( size_t* )createArray( sizeof( size_t ) );
    char** l_labels = ( char** )createArray( sizeof( char* ) );

    // Go over elements in order and register for rendering
    FOR_ARRAY( const char* const*, _elementsOrder ) {
        printf( "LABE %s\n", *_element );

        const ssize_t l_elementIndex =
            increaseElementCount( &l_labels, &l_labelCounts, *_element );
        const size_t l_labelCount =
            getElementCount( l_labels, l_labelCounts, *_element );

        // Insert element into overlay with settings or defalt settings
        {
            const size_t l_labelIndex = ( l_labelCount - 1 );
            char* l_labelIndexAsText = stoa( l_labelIndex );

            {
                char* l_elementLabelMangled = mangleElementLabel(
                    *_element, _overlayName, l_labelIndexAsText );

                {
                    const char* l_elementDefaultSettings =
                        getElementDefaultSettings( _elementsLabels,
                                                   _elementsSettings,
                                                   l_elementLabelMangled );
                    printf( "TEST4\n" );
                    char*** l_elementSettings = getLabelFromSettingsOrDefault(
                        l_elementLabelMangled, l_elementDefaultSettings );
                    printf( "TEST5\n" );

                    insertElementIntoOverlay(
                        &l_overlay, getElementTypeFromLabel( *_element ),
                        l_elementSettings );
                    printf( "TEST6\n" );
                    printf( "RE EL2 %s\n",
                            l_overlay[ arrayLength( l_overlay ) ]->text );
                }

                free( l_elementLabelMangled );
            }

            free( l_labelIndexAsText );
        }
    }

    insertIntoArray( ( void*** )&g_overlaysToRender, l_overlay );

    FREE_ARRAY( char* const*, l_labels, *_element );

    free( l_labelCounts );

    return ( l_returnValue );
}

static uint16_t freeElementsSettings( char*** _elementsLabels,
                                      char*** _elementsSettings ) {
    uint16_t l_returnValue = 0;

    FREE_ARRAY( char* const*, *_elementsLabels, *_element );
    FREE_ARRAY( char* const*, *_elementsSettings, *_element );

    return ( l_returnValue );
}

static uint16_t registerHotkey( const char* _overlayName,
                                const char* _overlayDefaultHotkey ) {
    uint16_t l_returnValue = 0;

    printf( "R HK2 %s\n", _overlayDefaultHotkey );
    const char l_overlayHotkeyName[] =
        "overlay_toggle_key"
        "_";
    char* l_overlayHotkeyNameMangled = strdup( l_overlayHotkeyName );

    concatBeforeAndAfterString( &l_overlayHotkeyNameMangled, "", _overlayName );

    {
        char* l_overlayHotkey = getKeyFromSettingsOrDefault(
            "keyboard", l_overlayHotkeyNameMangled, _overlayDefaultHotkey );

        printf( "LOHK %s\n", l_overlayHotkey );

        if ( strcmp( l_overlayHotkey, _overlayDefaultHotkey ) == 0 ) {
            _useCallback( "keyboard$reloadSettings" );
        }

        free( l_overlayHotkey );
    }

    printf( "R HK3 %s\n", l_overlayHotkeyNameMangled );
    insertIntoArray( ( void*** )&g_overlayHotkeys, l_overlayHotkeyNameMangled );

    return ( l_returnValue );
}

uint16_t overlayRegister( const char* _overlayName,
                          const char* const* _elementsOrder,
                          const char* _elementsDefaultSettings,
                          const uintptr_t* _elementsCallbackVariableReferences,
                          const char* _overlayDefaultHotkey ) {
    uint16_t l_returnValue = 0;

    char** l_elementsLabels = ( char** )createArray( sizeof( char* ) );
    char** l_elementsSettings = ( char** )createArray( sizeof( char* ) );

    printf( "TEST1\n" );
    if ( ( l_returnValue = getElementsSettings(
               &l_elementsLabels, &l_elementsSettings, _overlayName,
               _elementsDefaultSettings ) ) != 0 ) {
        goto FREE_LABELS;
    }

    printf( "TEST2\n" );
    if ( ( l_returnValue = registerElementsForRender(
               _overlayName, ( const char* const* )l_elementsLabels,
               _elementsOrder, ( const char* const* )l_elementsSettings ) ) !=
         0 ) {
        goto FREE_LABELS;
    }

    printf( "G %s\n", ( g_overlaysToRender[ 1 ][ 1 ] )->text );

    if ( ( l_returnValue =
               registerHotkey( _overlayName, _overlayDefaultHotkey ) ) != 0 ) {
        goto FREE_LABELS;
    }

    printf( "R HK1 %s\n", _overlayName );
    insertIntoArray( ( void*** )&g_overlayNames, ( void* )_overlayName );

FREE_LABELS:
    if ( ( l_returnValue = freeElementsSettings(
               &l_elementsLabels, &l_elementsSettings ) ) != 0 ) {
        goto EXIT;
    }

    printf( "G2 %s\n", ( g_overlaysToRender[ 1 ][ 1 ] )->text );

EXIT:
    return ( l_returnValue );
}
