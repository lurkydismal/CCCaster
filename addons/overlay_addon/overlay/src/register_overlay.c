#include "overlay.h"

#if 1
#include "_useCallback.h"
#endif

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "native.h"
#include "stdfunc.h"

static inline char* trim( const char* _text ) {
    const size_t l_textLength = strlen( _text );

    char* l_buffer = ( char* )malloc( ( l_textLength + 1 ) * sizeof( char ) );
    size_t l_bufferLength = 0;

    for ( const char* _symbol = _text; _symbol < ( _text + l_textLength );
          _symbol++ ) {
        if ( isspace( *_symbol ) ) {
            continue;

        } else if ( *_symbol == '#' ) {
            break;
        }

        l_buffer[ l_bufferLength ] = *_symbol;
        l_bufferLength++;
    }

    l_buffer[ l_bufferLength ] = '\0';
    l_bufferLength++;

    l_buffer = ( char* )realloc( l_buffer, l_bufferLength );

    return ( l_buffer );
}

static uint16_t getElementsSettings( char*** _elementsLabels,
                                     char*** _elementsSettings,
                                     const char* _overlayName,
                                     const char* _elementsDefaultSettings ) {
    uint16_t l_returnValue = 0;

    // Fill resort settings for overlay
    // In case of settings not having those already
    size_t* l_labelCounts = ( size_t* )createArray( sizeof( size_t ) );
    char** l_labels = ( char** )createArray( sizeof( char* ) );

    {
        char* l_text = strdup( _elementsDefaultSettings );

        const char l_delimiter[] = "\n";
        char* l_line = strtok( l_text, l_delimiter );
        char* l_buffer = ( char* )malloc( 1 );
        size_t l_bufferLength = 0;
        bool l_isFirstPass = true;

        while ( l_line ) {
            char* l_trimmedText = trim( l_line );
            const size_t l_textLength = strlen( l_trimmedText );

            if ( !l_textLength ) {
                goto PARSE_EXIT;
            }

            if ( l_trimmedText[ 0 ] == '[' ) {
                if ( !l_isFirstPass ) {
                    l_buffer =
                        ( char* )realloc( l_buffer, ( l_bufferLength + 1 ) );

                    l_buffer[ l_bufferLength ] = '\0';

                    insertIntoArray( ( void*** )_elementsSettings,
                                     strdup( l_buffer ) );

                    free( l_buffer );
                    l_buffer = ( char* )malloc( 1 );

                    l_bufferLength = 0;
                }

                {
                    l_trimmedText[ l_textLength - 1 ] = '\0';

                    char* l_label = ( char* )malloc( ( l_textLength + 1 ) *
                                                     sizeof( char ) );

                    memcpy( l_label, ( l_trimmedText + 1 ), l_textLength );

                    {
                        const size_t l_overlayNameLength =
                            strlen( _overlayName );
                        char* l_overlayNameWithUnderscore = ( char* )malloc(
                            ( l_overlayNameLength + 1 + 1 ) * sizeof( char ) );

                        memcpy( l_overlayNameWithUnderscore, _overlayName,
                                l_overlayNameLength );

                        l_overlayNameWithUnderscore[ l_overlayNameLength ] =
                            '_';
                        l_overlayNameWithUnderscore[ l_overlayNameLength + 1 ] =
                            '\0';

                        {
                            size_t l_labelCount;
                            const ssize_t l_labelIndex =
                                _findStringInArray( l_labels, l_label );

                            if ( l_labelIndex >= 1 ) {
                                ( l_labelCounts[ l_labelIndex ] )++;

                                l_labelCount = l_labelCounts[ l_labelIndex ];

                            } else {
                                insertIntoArray(
                                    ( void*** )&l_labels,
                                    ( void* )( strdup( l_label ) ) );
                                insertIntoArray( ( void*** )&l_labelCounts,
                                                 ( void* )1 );

                                l_labelCount = 1;
                            }

                            const size_t l_labelMangledIndex =
                                ( l_labelCount - 1 );
                            char* l_labelIndexAsText =
                                stoa( l_labelMangledIndex );

                            concatBeforeAndAfterString(
                                &l_label, l_overlayNameWithUnderscore,
                                l_labelIndexAsText );

                            insertIntoArray( ( void*** )_elementsLabels,
                                             strdup( l_label ) );

                            const size_t l_labelLength =
                                concatBeforeAndAfterString( &l_label, "[",
                                                            "]" );
                            printf( "LABEL %u %s\n", l_labelLength, l_label );

                            l_buffer = ( char* )realloc(
                                l_buffer, ( l_labelLength + 1 ) );
                            memcpy( ( l_buffer + l_bufferLength ), l_label,
                                    l_labelLength );
                            l_bufferLength += l_labelLength;

                            free( l_labelIndexAsText );
                        }

                        free( l_overlayNameWithUnderscore );
                    }

                    free( l_label );
                }

            } else {
                l_buffer = ( char* )realloc(
                    l_buffer, ( l_bufferLength + l_textLength + 1 ) );
                memcpy( ( l_buffer + l_bufferLength ), l_trimmedText,
                        l_textLength );
                l_bufferLength += l_textLength;
            }

            l_buffer[ l_bufferLength ] = '\n';
            l_bufferLength++;

            l_isFirstPass = false;

        PARSE_EXIT:
            free( l_trimmedText );

            printf( "LINE %s\n", l_line );
            l_line = strtok( NULL, l_delimiter );
        }

        if ( l_bufferLength == 0 ) {
            free( l_buffer );

        } else {
            l_buffer = ( char* )realloc( l_buffer, ( l_bufferLength + 1 ) );

            l_buffer[ l_bufferLength ] = '\0';

            insertIntoArray( ( void*** )_elementsSettings, strdup( l_buffer ) );

            free( l_buffer );

            l_bufferLength = 0;
        }

        free( l_text );
    }

    // Free l_labels
    {
        FOR_ARRAY( char* const*, l_labels ) {
            free( *_element );
        }

        free( l_labels );
    }

    free( l_labelCounts );

    return ( l_returnValue );
}

static uint16_t registerElementsForRender(
    const char* _overlayName,
    const char* const* _elementsLabels,
    const char* const* _elementsOrder,
    const char* const* _elementsSettings ) {
    uint16_t l_returnValue = 0;

    element_t** l_overlay = ( element_t** )createArray( sizeof( element_t* ) );
    // Go over elements in order
    // And register those for rendering
    size_t* l_labelCounts = ( size_t* )createArray( sizeof( size_t ) );
    char** l_labels = ( char** )createArray( sizeof( char* ) );

    FOR_ARRAY( const char* const*, _elementsOrder ) {
        printf( "LABE %s\n", *_element );

        // Register for rendering
        {
            size_t l_labelCount;
            const ssize_t l_labelIndex =
                _findStringInArray( l_labels, *_element );

            if ( l_labelIndex >= 1 ) {
                ( l_labelCounts[ l_labelIndex ] )++;

                l_labelCount = l_labelCounts[ l_labelIndex ];

            } else {
                insertIntoArray( ( void*** )&l_labels,
                                 ( void* )( strdup( *_element ) ) );
                insertIntoArray( ( void*** )&l_labelCounts, ( void* )1 );

                l_labelCount = 1;
            }

            const size_t l_labelMangledIndex = ( l_labelCount - 1 );
            char* l_labelIndexAsText = stoa( l_labelMangledIndex );

            {
                char* l_overlayNameMangled = strdup( _overlayName );
                const size_t l_overlayNameMangledLength =
                    concatBeforeAndAfterString( &l_overlayNameMangled, "",
                                                "_" );

                const size_t l_elementNameIndexAsTextMangledLength =
                    concatBeforeAndAfterString( &l_labelIndexAsText, *_element,
                                                "" );

                const size_t l_labelMangledLength =
                    ( l_overlayNameMangledLength +
                      l_elementNameIndexAsTextMangledLength );
                {
                    char* l_labelMangled = ( char* )malloc(
                        ( l_labelMangledLength + 1 ) * sizeof( char ) );
                    memcpy( l_labelMangled, l_overlayNameMangled,
                            l_overlayNameMangledLength );
                    memcpy( ( l_labelMangled + l_overlayNameMangledLength ),
                            l_labelIndexAsText,
                            l_elementNameIndexAsTextMangledLength );
                    l_labelMangled[ l_labelMangledLength ] = '\0';

                    {
                        char*** l_elementSettings;

                        if ( _useCallback( "core$getSettingsContentByLabel",
                                           &l_elementSettings,
                                           l_labelMangled ) != 0 ) {
                            const char* l_elementDefaultSettings;

                            const size_t l_elementDefaultSettingsIndex =
                                _findStringInArray( _elementsLabels,
                                                    l_labelMangled );
                            printf( "ELEM %s\n", _elementsLabels[ 1 ] );

                            if ( l_elementDefaultSettingsIndex >= 1 ) {
                                l_elementDefaultSettings = _elementsSettings
                                    [ l_elementDefaultSettingsIndex ];
                            }

                            printf( "LABE1 %d\n",
                                    l_elementDefaultSettingsIndex );
                            printf( "LABE2 %s\n", l_elementDefaultSettings );
                            _useCallback( "core$readSettingsFromString",
                                          l_elementDefaultSettings );

                            _useCallback( "core$getSettingsContentByLabel",
                                          &l_elementSettings, l_labelMangled );
                            printf( "LABE3 %s\n", l_labelMangled );
                            printf( "LABE4 %p\n", l_elementSettings );
                        }

                        printf( "LABE %s\n", *_element );
                        {
                            element_t l_element = DEFAULT_ELEMENT_PARAMETERS;
                            const char* l_label = *_element;

                            FOR( char* const*, g_elementTypesAsString ) {
                                if ( strcmp( *_element, l_label ) == 0 ) {
                                    l_element.type =
                                        ( _element - g_elementTypesAsString );

                                    break;
                                }
                            }

                            printf( "LABE %s\n", *_element );
                            FOR_ARRAY( char** const*, l_elementSettings ) {
                                char* l_key = ( *_element )[ 0 ];
                                char* l_value = ( *_element )[ 1 ];

                                if ( strcmp( l_key, "x" ) == 0 ) {
                                    l_element.coordinates.x = atol( l_value );

                                } else if ( strcmp( l_key, "y" ) == 0 ) {
                                    l_element.coordinates.y = atol( l_value );

                                } else if ( strcmp( l_key, "width" ) == 0 ) {
                                    l_element.size.width = atol( l_value );

                                } else if ( strcmp( l_key, "height" ) == 0 ) {
                                    l_element.size.height = atol( l_value );

                                } else if ( strcmp( l_key, "red" ) == 0 ) {
                                    const uint8_t l_red = atol( l_value );

                                    l_element.a.red = l_red;

                                    if ( l_element.type == RECTANGLE ) {
                                        l_element.b.red = l_red;
                                        l_element.c.red = l_red;
                                        l_element.d.red = l_red;
                                    }

                                } else if ( strcmp( l_key, "a_red" ) == 0 ) {
                                    const uint8_t l_red = atol( l_value );

                                    l_element.a.red = l_red;

                                } else if ( strcmp( l_key, "b_red" ) == 0 ) {
                                    const uint8_t l_red = atol( l_value );

                                    l_element.b.red = l_red;

                                } else if ( strcmp( l_key, "c_red" ) == 0 ) {
                                    const uint8_t l_red = atol( l_value );

                                    l_element.c.red = l_red;

                                } else if ( strcmp( l_key, "d_red" ) == 0 ) {
                                    const uint8_t l_red = atol( l_value );

                                    l_element.d.red = l_red;

                                } else if ( strcmp( l_key, "green" ) == 0 ) {
                                    const uint8_t l_green = atol( l_value );

                                    l_element.a.green = l_green;

                                    if ( l_element.type == RECTANGLE ) {
                                        l_element.b.green = l_green;
                                        l_element.c.green = l_green;
                                        l_element.d.green = l_green;
                                    }

                                } else if ( strcmp( l_key, "a_green" ) == 0 ) {
                                    const uint8_t l_green = atol( l_value );

                                    l_element.a.green = l_green;

                                } else if ( strcmp( l_key, "b_green" ) == 0 ) {
                                    const uint8_t l_green = atol( l_value );

                                    l_element.b.green = l_green;

                                } else if ( strcmp( l_key, "c_green" ) == 0 ) {
                                    const uint8_t l_green = atol( l_value );

                                    l_element.c.green = l_green;

                                } else if ( strcmp( l_key, "d_green" ) == 0 ) {
                                    const uint8_t l_green = atol( l_value );

                                    l_element.d.green = l_green;

                                } else if ( strcmp( l_key, "blue" ) == 0 ) {
                                    const uint8_t l_blue = atol( l_value );

                                    l_element.a.blue = l_blue;

                                    if ( l_element.type == RECTANGLE ) {
                                        l_element.b.blue = l_blue;
                                        l_element.c.blue = l_blue;
                                        l_element.d.blue = l_blue;
                                    }

                                } else if ( strcmp( l_key, "a_blue" ) == 0 ) {
                                    const uint8_t l_blue = atol( l_value );

                                    l_element.a.blue = l_blue;

                                } else if ( strcmp( l_key, "b_blue" ) == 0 ) {
                                    const uint8_t l_blue = atol( l_value );

                                    l_element.b.blue = l_blue;

                                } else if ( strcmp( l_key, "c_blue" ) == 0 ) {
                                    const uint8_t l_blue = atol( l_value );

                                    l_element.c.blue = l_blue;

                                } else if ( strcmp( l_key, "d_blue" ) == 0 ) {
                                    const uint8_t l_blue = atol( l_value );

                                    l_element.d.blue = l_blue;

                                } else if ( strcmp( l_key, "alpha" ) == 0 ) {
                                    const uint8_t l_alpha = atol( l_value );

                                    l_element.a.alpha = l_alpha;

                                    if ( l_element.type == RECTANGLE ) {
                                        l_element.b.alpha = l_alpha;
                                        l_element.c.alpha = l_alpha;
                                        l_element.d.alpha = l_alpha;
                                    }

                                } else if ( strcmp( l_key, "a_alpha" ) == 0 ) {
                                    const uint8_t l_alpha = atol( l_value );

                                    l_element.a.alpha = l_alpha;

                                } else if ( strcmp( l_key, "b_alpha" ) == 0 ) {
                                    const uint8_t l_alpha = atol( l_value );

                                    l_element.b.alpha = l_alpha;

                                } else if ( strcmp( l_key, "c_alpha" ) == 0 ) {
                                    const uint8_t l_alpha = atol( l_value );

                                    l_element.c.alpha = l_alpha;

                                } else if ( strcmp( l_key, "d_alpha" ) == 0 ) {
                                    const uint8_t l_alpha = atol( l_value );

                                    l_element.d.alpha = l_alpha;

                                } else if ( strcmp( l_key, "text" ) == 0 ) {
                                    l_element.text = l_value;

                                } else if ( strcmp( l_key, "shade_first" ) ==
                                            0 ) {
                                    l_element.shade.first = atol( l_value );

                                } else if ( strcmp( l_key, "shade_second" ) ==
                                            0 ) {
                                    l_element.shade.second = atol( l_value );

                                } else if ( strcmp( l_key, "letter_spacing" ) ==
                                            0 ) {
                                    l_element.letterSpacing = atol( l_value );

                                } else if ( strcmp( l_key, "layer" ) == 0 ) {
                                    l_element.layer = atol( l_value );
                                }
                            }

                            element_t* l_elementClone =
                                ( element_t* )malloc( sizeof( l_element ) );
                            memcpy( l_elementClone, &l_element,
                                    sizeof( l_element ) );

                            printf( "RE EL %s\n", l_element.text );
                            insertIntoArray( ( void*** )&l_overlay,
                                             ( void* )( l_elementClone ) );
                            printf(
                                "RE EL2 %s\n",
                                l_overlay[ arrayLength( l_overlay ) ]->text );
                        }

                        free( l_elementSettings );
                    }

                    free( l_labelMangled );
                }

                free( l_overlayNameMangled );
            }

            free( l_labelIndexAsText );
        }
    }

    insertIntoArray( ( void*** )&g_overlaysToRender, l_overlay );

    // Free l_labels
    {
        FOR_ARRAY( char* const*, l_labels ) {
            free( *_element );
        }

        free( l_labels );
    }

    free( l_labelCounts );

    return ( l_returnValue );
}

static uint16_t freeElementsSettings( char*** _elementsLabels,
                                      char*** _elementsSettings ) {
    uint16_t l_returnValue = 0;

    // Free labels
    {
        char** l_content = *_elementsLabels;

        FOR_ARRAY( char* const*, *_elementsLabels ) {
            printf( "{\n", *_element );
            printf( "%s\n", *_element );
            printf( "}\n", *_element );

            free( *_element );
        }

        free( *_elementsLabels );
    }

    // Free settings
    {
        FOR_ARRAY( char* const*, *_elementsSettings ) {
            printf( "{\n", *_element );
            printf( "%s", *_element );
            printf( "}\n", *_element );

            free( *_element );
        }

        free( *_elementsSettings );
    }

    return ( l_returnValue );
}

static uint16_t registerHotkey( const char* _overlayName,
                                const char* _overlayDefaultHotkey ) {
    uint16_t l_returnValue = 0;

    printf( "R HK2 %s\n", _overlayDefaultHotkey );
    const char l_overlayHotkeyName[] = "overlay_toggle_key";
    const size_t l_overlayHotkeyNameLength = strlen( l_overlayHotkeyName );
    const size_t l_overlayNameLength = strlen( _overlayName );
    const size_t l_overlayHotkeyNameMangledTotalLength =
        ( l_overlayHotkeyNameLength + 1 + l_overlayNameLength );
    char* l_overlayHotkeyNameMangled = ( char* )malloc(
            ( l_overlayHotkeyNameMangledTotalLength + 1 ) * sizeof( char ) );
    size_t l_overlayHotkeyNameMangledLength = 0;

    memcpy( l_overlayHotkeyNameMangled, l_overlayHotkeyName,
            l_overlayHotkeyNameLength );
    l_overlayHotkeyNameMangledLength += l_overlayHotkeyNameLength;

    l_overlayHotkeyNameMangled[ l_overlayHotkeyNameMangledLength ] = '_';
    l_overlayHotkeyNameMangledLength += 1;

    memcpy(
            ( l_overlayHotkeyNameMangled + l_overlayHotkeyNameMangledLength ),
            _overlayName, l_overlayNameLength );
    l_overlayHotkeyNameMangledLength += l_overlayNameLength;

    l_overlayHotkeyNameMangled[ l_overlayHotkeyNameMangledLength ] = '\0';
    l_overlayHotkeyNameMangledLength += 1;

    {
        char*** l_settings;

        if ( ( l_returnValue =
                    _useCallback( "core$getSettingsContentByLabel",
                        &l_settings, _overlayName ) ) == 0 ) {
            const ssize_t l_overlayHotkeyIndex = findKeyInSettings(
                    l_settings, l_overlayHotkeyNameMangled );

            if ( l_overlayHotkeyIndex == -1 ) {
                if ( _useCallback( "core$changeSettingsKeyByLabel",
                            l_overlayHotkeyNameMangled,
                            "keyboard",
                            _overlayDefaultHotkey ) != 1 ) {
                    _useCallback( "keyboard$reloadSettings" );
                }
            }

            freeSettingsContent( l_settings );
        }
    }

    printf( "R HK3 %s\n", l_overlayHotkeyNameMangled );
    insertIntoArray( ( void*** )&g_overlayHotkeys,
            l_overlayHotkeyNameMangled );

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

    if ( ( l_returnValue = getElementsSettings(
               &l_elementsLabels, &l_elementsSettings, _overlayName,
               _elementsDefaultSettings ) ) != 0 ) {
        goto FREE_LABELS;
    }

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
