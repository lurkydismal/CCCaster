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
#if 0
    printf( "getElementTypeFromLabel %s\n", _elementLabel );
#endif
    FOR( char* const*, g_elementTypesAsString ) {
        if ( strcmp( *_element, _elementLabel ) == 0 ) {
#if 0
            printf( "LT %s %d\n", _elementLabel,
                    ( _element - g_elementTypesAsString ) );
#endif
            return ( _element - g_elementTypesAsString );
        }
    }
}

static inline const char* getElementLabelFromType(
    const enum elementType _elementType ) {
#if 0
    printf( "getElementLabelFromType %d\n", _elementType );
#endif
#if 0
    printf( "LT %d %s\n", _elementType,
            g_elementTypesAsString[ _elementType ] );
#endif

    return ( g_elementTypesAsString[ _elementType ] );
}

static ssize_t increaseElementCount( char*** _elementLabels,
                                     size_t** _countsArray,
                                     const char* _label ) {
    const ssize_t l_labelIndex = _findStringInArray( *_elementLabels, _label );
    printf( "EL IEC %s\n", _label );
    printf( "EL IECI %d\n", l_labelIndex );

    if ( l_labelIndex >= 1 ) {
        ( *_countsArray[ l_labelIndex ] )++;

    } else {
        insertIntoArray( ( void*** )_elementLabels,
                         ( void* )( strdup( _label ) ) );
        insertIntoArray( ( void*** )_countsArray, ( void* )1 );
    }

    return ( l_labelIndex );
}

static inline ssize_t getElementCount( char** _elementLabels,
                                       size_t* _countsArray,
                                       const char* _label ) {
    ssize_t l_returnValue = -1;
    const ssize_t l_labelIndex = _findStringInArray( _elementLabels, _label );
    printf( "EL C %s\n", _label );
    printf( "EL CI %d\n", l_labelIndex );

    if ( l_labelIndex >= 1 ) {
        l_returnValue = _countsArray[ l_labelIndex ];
    }

    printf( "EL C %d\n", l_returnValue );

    return ( l_returnValue );
}

static inline char* mangleElementLabel( const char* _label,
                                        const char* _overlayName,
                                        const char* _labelIndexAsText ) {
    char* l_returnValue = strdup( "_" );

    concatBeforeAndAfterString( &l_returnValue, _overlayName, _label );
    concatBeforeAndAfterString( &l_returnValue, "", _labelIndexAsText );

    return ( l_returnValue );
}

static inline bool isLabel( const char* _string, const size_t _stringLength ) {
    return ( ( _string[ 0 ] == '[' ) &&
             ( _string[ _stringLength - 1 ] == ']' ) );
}

static uint16_t getElementsSettings( char*** _elementsLabels,
                                     char*** _elementsSettings,
                                     const char* _overlayName,
                                     const char* _elementsDefaultSettings ) {
    uint16_t l_returnValue = 0;

    size_t* l_labelCounts = ( size_t* )createArray( sizeof( size_t ) );
    char** l_labels = ( char** )createArray( sizeof( char* ) );

    {
        const char l_delimiter[] = "\n";
        char** l_elementsDefaultSettings =
            splitStringIntoArray( _elementsDefaultSettings, l_delimiter );

        char* l_buffer = strdup( "" );
        size_t l_bufferLength = 0;

#define moveBufferIntoElementsSettings()                                     \
    do {                                                                     \
        l_buffer = ( char* )realloc( l_buffer, ( l_bufferLength + 1 ) );     \
        insertIntoArray( ( void*** )_elementsSettings, strdup( l_buffer ) ); \
        free( l_buffer );                                                    \
        l_bufferLength = 0;                                                  \
    } while ( false )

        bool l_isFirstPass = true;

        FOR_ARRAY( char* const*, l_elementsDefaultSettings ) {
            char* l_line = sanitizeString( *_element );
            const size_t l_lineLength = strlen( l_line );

            if ( !l_lineLength ) {
                goto PARSE_EXIT;
            }

            // Label
            if ( isLabel( l_line, l_lineLength ) ) {
                if ( !l_isFirstPass ) {
                    moveBufferIntoElementsSettings();
                    l_buffer = strdup( "" );
                }

                trim( l_line, 1, ( l_lineLength - 2 ) );

                const ssize_t l_elementIndex =
                    increaseElementCount( &l_labels, &l_labelCounts, l_line );
                const ssize_t l_labelCount =
                    getElementCount( l_labels, l_labelCounts, l_line );

                {
                    const size_t l_labelIndex = ( l_labelCount - 1 );
                    char* l_labelIndexAsText = stoa( l_labelIndex );
                    printf( "TESTTTT %s\n", l_labelIndexAsText );

                    {
                        char* l_label = mangleElementLabel(
                            l_line, _overlayName, l_labelIndexAsText );

                        insertIntoArray( ( void*** )_elementsLabels,
                                         strdup( l_label ) );

                        concatBeforeAndAfterString( &l_label, "[", "]" );

                        concatBeforeAndAfterString( &l_buffer, "", l_label );
#if 0
                        printf( "LB1 %s %d\n", l_buffer, strlen( l_buffer ) );
#endif

                        free( l_label );
                    }

                    free( l_labelIndexAsText );
                }

                // Not Label
            } else {
                concatBeforeAndAfterString( &l_buffer, "", l_line );
#if 0
                printf( "LB2 %s %d\n", l_line, strlen( l_line ) );
#endif
            }

            l_bufferLength = concatBeforeAndAfterString( &l_buffer, "", "\n" );

            l_isFirstPass = false;

        PARSE_EXIT:
            free( l_line );

#if 0
            printf( "LINE %s\n", l_line );
#endif
        }

        if ( l_bufferLength != 0 ) {
            moveBufferIntoElementsSettings();
        }

        FREE_ARRAY( char**, l_elementsDefaultSettings, *_element );

#undef moveBufferIntoElementsSettings
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
#if 0
    printf( "ELEM %s\n", _elementsLabels[ 1 ] );
#endif
    const size_t l_elementDefaultSettingsIndex =
        _findStringInArray( _elementsLabels, _elementLabel );

    if ( l_elementDefaultSettingsIndex >= 1 ) {
        l_returnValue = _elementsSettings[ l_elementDefaultSettingsIndex ];
    }

#if 0
    printf( "LABE1 %d\n", l_elementDefaultSettingsIndex );
#endif
#if 0
    printf( "LABE2 %s\n", _elementsSettings[ 1 ] );
#endif

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

static inline void insertElementIntoOverlay( element_t*** _overlay,
                                             const enum elementType _type,
                                             char*** _settings ) {
    element_t* l_element = createElementWithSettings( _type, _settings );

#if 0
    printf( "RE EL %s\n", l_element->text );
#endif
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
#if 0
        printf( "LABE %s\n", *_element );
#endif

        const ssize_t l_elementIndex =
            increaseElementCount( &l_labels, &l_labelCounts, *_element );
        const size_t l_labelCount =
            getElementCount( l_labels, l_labelCounts, *_element );

        // Insert element into overlay with settings or defalt settings
        {
            const size_t l_labelIndex = ( l_labelCount - 1 );
            char* l_labelIndexAsText = stoa( l_labelIndex );
            printf( "TESTTTT %s\n", l_labelIndexAsText );

            {
                char* l_elementLabelMangled = mangleElementLabel(
                    *_element, _overlayName, l_labelIndexAsText );

                {
                    const char* l_elementDefaultSettings =
                        getElementDefaultSettings( _elementsLabels,
                                                   _elementsSettings,
                                                   l_elementLabelMangled );
#if 0
                    printf( "TEST4\n" );
#endif
                    char*** l_elementSettings = getLabelFromSettingsOrDefault(
                        l_elementLabelMangled, l_elementDefaultSettings );
#if 0
                    printf( "TEST5\n" );
#endif

                    insertElementIntoOverlay(
                        &l_overlay, getElementTypeFromLabel( *_element ),
                        l_elementSettings );
#if 0
                    printf( "TEST6\n" );
#endif
#if 0
                    printf( "RE EL2 %s\n",
                            l_overlay[ arrayLength( l_overlay ) ]->text );
#endif
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

#if 0
    printf( "R HK2 %s\n", _overlayDefaultHotkey );
#endif
    const char l_overlayHotkeyName[] =
        "overlay_toggle_key"
        "_";
    char* l_overlayHotkeyNameMangled = strdup( l_overlayHotkeyName );

    concatBeforeAndAfterString( &l_overlayHotkeyNameMangled, "", _overlayName );

    {
        char* l_overlayHotkey = getKeyFromSettingsOrDefault(
            "keyboard", l_overlayHotkeyNameMangled, _overlayDefaultHotkey );

#if 0
        printf( "LOHK %s\n", l_overlayHotkey );
#endif

        if ( strcmp( l_overlayHotkey, _overlayDefaultHotkey ) == 0 ) {
            _useCallback( "keyboard$reloadSettings" );
        }

        free( l_overlayHotkey );
    }

#if 0
    printf( "R HK3 %s\n", l_overlayHotkeyNameMangled );
#endif
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

#if 0
    printf( "G %s\n", ( g_overlaysToRender[ 1 ][ 1 ] )->text );
#endif

    if ( ( l_returnValue =
               registerHotkey( _overlayName, _overlayDefaultHotkey ) ) != 0 ) {
        goto FREE_LABELS;
    }

#if 0
    printf( "R HK1 %s\n", _overlayName );
#endif
    insertIntoArray( ( void*** )&g_overlayNames, ( void* )_overlayName );

FREE_LABELS:
    if ( ( l_returnValue = freeElementsSettings(
               &l_elementsLabels, &l_elementsSettings ) ) != 0 ) {
        goto EXIT;
    }

#if 0
    printf( "G2 %s\n", ( g_overlaysToRender[ 1 ][ 1 ] )->text );
#endif

EXIT:
    return ( l_returnValue );
}
