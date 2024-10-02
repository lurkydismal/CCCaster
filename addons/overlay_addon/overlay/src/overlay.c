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
                            size_t l_itemCount;
                            const ssize_t l_itemNameIndex =
                                _findStringInArray( l_labels, l_label );

                            if ( l_itemNameIndex >= 1 ) {
                                ( l_labelCounts[ l_itemNameIndex ] )++;

                                l_itemCount = l_labelCounts[ l_itemNameIndex ];

                            } else {
                                insertIntoArray(
                                    ( void*** )&l_labels,
                                    ( void* )( strdup( l_label ) ) );
                                insertIntoArray( ( void*** )&l_labelCounts,
                                                 ( void* )1 );

                                l_itemCount = 1;
                            }

                            const size_t l_itemIndex = ( l_itemCount - 1 );
                            char* l_itemIndexAsText = stoa( l_itemIndex );

                            concatBeforeAndAfterString(
                                &l_label, l_overlayNameWithUnderscore,
                                l_itemIndexAsText );

                            const size_t l_labelLength =
                                concatBeforeAndAfterString( &l_label, "[",
                                                            "]" );
                            printf( "LABEL %u %s\n", l_labelLength, l_label );

                            insertIntoArray( ( void*** )_elementsLabels,
                                             strdup( l_label ) );

                            l_buffer = ( char* )realloc(
                                l_buffer, ( l_labelLength + 1 ) );
                            memcpy( ( l_buffer + l_bufferLength ), l_label,
                                    l_labelLength );
                            l_bufferLength += l_labelLength;

                            free( l_itemIndexAsText );
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
        char* const* l_content = l_labels;
        const size_t l_contentLength = arrayLength( l_content );
        char* const* l_contentFirstElement =
            arrayFirstElementPointer( l_content );
        char* const* l_contentEnd = ( l_contentFirstElement + l_contentLength );

        for ( char* const* _label = l_contentFirstElement;
              _label < l_contentEnd; _label++ ) {
            free( *_label );
        }

        free( l_labels );
    }

    free( l_labelCounts );

    return ( l_returnValue );
}

#if 0
static void parseLayout( const char* _overlayName,
                         const char* const* _overlayItems,
                         const char* _overlay ) {
    // Go over items in order
    // And register those for rendering
    {
        char** l_itemNames = ( char** )createArray( sizeof( char* ) );
        size_t* l_itemCounts = ( size_t* )createArray( sizeof( size_t ) );

        const char* const* l_content = _overlayItems;
        const size_t l_contentLength = arrayLength( l_content );
        const char* const* l_contentFirstElement =
            arrayFirstElementPointer( l_content );
        const char* const* l_contentEnd =
            ( l_contentFirstElement + l_contentLength );

        for ( const char* const* _item = l_contentFirstElement;
              _item != l_contentEnd; _item++ ) {
            const size_t l_textLength = strlen( *_item );

            char* l_label =
                ( char* )malloc( ( l_textLength + 1 ) * sizeof( char ) );

            memcpy( l_label, ( *_item ), ( l_textLength + 1 ) );

            {
                const size_t l_overlayNameLength = strlen( _overlayName );
                char* l_overlayNameWithUnderscore = ( char* )malloc(
                    ( l_overlayNameLength + 1 + 1 ) * sizeof( char ) );

                memcpy( l_overlayNameWithUnderscore, _overlayName,
                        l_overlayNameLength );

                l_overlayNameWithUnderscore[ l_overlayNameLength ] = '_';
                l_overlayNameWithUnderscore[ l_overlayNameLength + 1 ] = '\0';

                {
                    size_t l_itemCount;
                    const ssize_t l_itemNameIndex =
                        _findStringInArray( l_itemNames, *_item );

                    if ( l_itemNameIndex >= 1 ) {
                        ( l_itemCounts[ l_itemNameIndex ] )++;

                        l_itemCount = l_itemCounts[ l_itemNameIndex ];

                    } else {
                        insertIntoArray( ( void*** )&l_itemNames,
                                         ( void* )( *_item ) );
                        insertIntoArray( ( void*** )&l_itemCounts, ( void* )1 );

                        l_itemCount = 1;
                    }

                    const size_t l_itemIndex = ( l_itemCount - 1 );
                    char* l_itemIndexAsText = stoa( l_itemIndex );

                    concatBeforeAndAfterString( &l_label,
                                                l_overlayNameWithUnderscore,
                                                l_itemIndexAsText );

                    free( l_itemIndexAsText );

                    char* l_labelWithBrackets = strdup( l_label );
                    concatBeforeAndAfterString( &l_labelWithBrackets, "[",
                                                "]" );

                    // Register for rendering
                    {
                        char*** l_itemSettings;

                        if ( _useCallback( "core$getSettingsContentByLabel",
                                           &l_itemSettings, l_label ) != 0 ) {
                            char* l_itemDefaultSettings;

                            const size_t l_itemDefaultSettingsIndex =
                                _findStringInArray( l_overlayLabels,
                                                    l_labelWithBrackets );

                            if ( l_itemDefaultSettingsIndex >= 1 ) {
                                l_itemDefaultSettings =
                                    l_overlay[ l_itemDefaultSettingsIndex ];
                            }

                            _useCallback( "core$readSettingsFromString",
                                          l_itemDefaultSettings );

                            _useCallback( "core$getSettingsContentByLabel",
                                          &l_itemSettings, l_label );
                        }

                        free( l_labelWithBrackets );

                        {
                            element_t l_element = DEFAULT_ELEMENT_PARAMETERS;

                            {
                                {
                                    char** l_content = g_elementTypesAsString;
                                    const size_t l_contentLength =
                                        arrayLength( l_content );
                                    char* const* l_contentFirstElement =
                                        l_content;
                                    char* const* l_contentEnd =
                                        ( l_contentFirstElement +
                                          l_contentLength );

                                    for ( char* const* _type =
                                              l_contentFirstElement;
                                          ( ( _type != l_contentEnd ) &&
                                            ( *_type != NULL ) );
                                          _type++ ) {
                                        if ( strcmp( *_type, *_item ) == 0 ) {
                                            l_element.type =
                                                ( _type -
                                                  l_contentFirstElement );

                                            break;
                                        }
                                    }
                                }
                            }

                            {
                                char*** l_content = l_itemSettings;
                                const size_t l_contentLength =
                                    arrayLength( l_content );
                                char** const* l_contentFirstElement =
                                    arrayFirstElementPointer( l_content );
                                char** const* l_contentEnd =
                                    ( l_contentFirstElement + l_contentLength );

                                for ( char** const* _pair =
                                          l_contentFirstElement;
                                      _pair != l_contentEnd; _pair++ ) {
                                    char* l_key = ( *_pair )[ 0 ];
                                    char* l_value = ( *_pair )[ 1 ];

                                    if ( strcmp( l_key, "x" ) == 0 ) {
                                        l_element.coordinates.x =
                                            atol( l_value );

                                    } else if ( strcmp( l_key, "y" ) == 0 ) {
                                        l_element.coordinates.y =
                                            atol( l_value );

                                    } else if ( strcmp( l_key, "width" ) ==
                                                0 ) {
                                        l_element.size.width = atol( l_value );

                                    } else if ( strcmp( l_key, "height" ) ==
                                                0 ) {
                                        l_element.size.height = atol( l_value );

                                    } else if ( strcmp( l_key, "red" ) == 0 ) {
                                        l_element.red = atol( l_value );

                                    } else if ( strcmp( l_key, "green" ) ==
                                                0 ) {
                                        l_element.green = atol( l_value );

                                    } else if ( strcmp( l_key, "blue" ) == 0 ) {
                                        l_element.blue = atol( l_value );

                                    } else if ( strcmp( l_key, "alpha" ) ==
                                                0 ) {
                                        l_element.alpha = atol( l_value );

                                    } else if ( strcmp( l_key, "text" ) == 0 ) {
                                        // TODO: strdup
                                        l_element.text = l_value;
                                    }
                                }
                            }

                            insertIntoArray( ( void*** )&g_elementsToRender,
                                             &l_element );
                        }
                    }
                }

                free( l_overlayNameWithUnderscore );
            }

            free( l_label );
        }

        free( l_itemNames );
        free( l_itemCounts );
    }
}
#endif

static uint16_t freeElementsSettings( char*** _elementsLabels,
                                      char*** _elementsSettings ) {
    uint16_t l_returnValue = 0;

    {
        char** l_content = *_elementsLabels;
        const size_t l_contentLength = arrayLength( l_content );
        char* const* l_contentFirstElement =
            arrayFirstElementPointer( l_content );
        char* const* l_contentEnd = ( l_contentFirstElement + l_contentLength );

        for ( char* const* _item = l_contentFirstElement; _item != l_contentEnd;
              _item++ ) {
            printf( "{\n", *_item );
            printf( "%s\n", *_item );
            printf( "}\n", *_item );

            free( *_item );
        }

        free( *_elementsLabels );
    }

    {
        char** l_content = *_elementsSettings;
        const size_t l_contentLength = arrayLength( l_content );
        char* const* l_contentFirstElement =
            arrayFirstElementPointer( l_content );
        char* const* l_contentEnd = ( l_contentFirstElement + l_contentLength );

        for ( char* const* _item = l_contentFirstElement; _item != l_contentEnd;
              _item++ ) {
            printf( "{\n", *_item );
            printf( "%s", *_item );
            printf( "}\n", *_item );

            free( *_item );
        }

        free( *_elementsSettings );
    }

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
        l_returnValue = 1;

        goto FREE_LABELS;
    }

#if 0
    if ( ( l_returnValue = registerElementsForRender(
               l_elementsLabels, _elementsOrder, l_elementsSettings ) ) != 0 ) {
        l_returnValue = 1;

        goto FREE_LABELS;
    }
#endif

#if 0
    l_returnValue = registerHotkey( _overlayName, _overlayDefaultHotkey );
#endif

FREE_LABELS:
    if ( ( l_returnValue = freeElementsSettings(
               &l_elementsLabels, &l_elementsSettings ) ) != 0 ) {
        l_returnValue = 1;

        goto EXIT;
    }

EXIT:
    return ( l_returnValue );
}
