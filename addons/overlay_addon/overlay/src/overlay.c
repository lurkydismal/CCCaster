#include "overlay.h"

#if 1
#include "_useCallback.h"
#endif

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "stdfunc.h"

static inline char* trim( const char* _text ) {
    const size_t l_textLength = strlen( _text );

    char* l_buffer = ( char* )malloc( ( l_textLength + 1 ) * sizeof( char ) );
    size_t l_bufferLength = 0;

    for ( size_t _index = 0; _index < l_textLength; _index++ ) {
        const char l_symbol = _text[ _index ];

        if ( isspace( l_symbol ) ) {
            continue;

        } else if ( l_symbol == '#' ) {
            break;
        }

        l_buffer[ l_bufferLength ] = l_symbol;
        l_bufferLength++;
    }

    l_buffer[ l_bufferLength ] = '\0';
    l_bufferLength++;

    l_buffer = ( char* )realloc( l_buffer, l_bufferLength );

    return ( l_buffer );
}

static void parseLayout( const char* _overlayName,
                         const char* const* _overlayItems,
                         const char* _overlay ) {
    char** l_overlayLabels = ( char** )createArray( sizeof( char* ) );
    char** l_overlay = ( char** )createArray( sizeof( char* ) );

    // Fill resort settings for overlay
    // In case of settings not having those already
    {
        char** l_labels = ( char** )createArray( sizeof( char* ) );
        size_t* l_labelCounts = ( size_t* )createArray( sizeof( size_t ) );

        char* l_text = strdup( _overlay );

        const char l_delimiter[] = "\n";
        char* l_line = strtok( l_text, l_delimiter );
        char* l_buffer = ( char* )malloc( 1 );
        size_t l_bufferLength = 0;
        bool l_isFirstPass = true;

        while ( l_line ) {
            {
                char* l_trimmedText = trim( l_line );
                const size_t l_textLength = strlen( l_trimmedText );

                if ( !l_textLength ) {
                    goto PARSE_EXIT;
                }

                if ( l_trimmedText[ 0 ] == '[' ) {
                    if ( !l_isFirstPass ) {
                        l_buffer = ( char* )realloc( l_buffer,
                                                     ( l_bufferLength + 1 ) );

                        l_buffer[ l_bufferLength ] = '\0';

                        insertIntoArray( ( void*** )&l_overlay,
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
                                ( l_overlayNameLength + 1 + 1 ) *
                                sizeof( char ) );

                            memcpy( l_overlayNameWithUnderscore, _overlayName,
                                    l_overlayNameLength );

                            l_overlayNameWithUnderscore[ l_overlayNameLength ] =
                                '_';
                            l_overlayNameWithUnderscore[ l_overlayNameLength +
                                                         1 ] = '\0';

                            {
                                size_t l_itemCount;
                                const ssize_t l_itemNameIndex =
                                    _findStringInArray( l_labels, l_label );

                                if ( l_itemNameIndex >= 1 ) {
                                    ( l_labelCounts[ l_itemNameIndex ] )++;

                                    l_itemCount =
                                        l_labelCounts[ l_itemNameIndex ];

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
                                printf( "LABEL %u %s\n", l_labelLength,
                                        l_label );

                                insertIntoArray( ( void*** )&l_overlayLabels,
                                                 strdup( l_label ) );

                                l_buffer = ( char* )realloc(
                                    l_buffer, ( l_labelLength + 1 ) );
                                memcpy( ( l_buffer + l_bufferLength ), l_label,
                                        l_labelLength );
                                l_bufferLength += l_labelLength;
                                l_buffer[ l_bufferLength ] = '\n';
                                l_bufferLength++;

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
                    l_buffer[ l_bufferLength ] = '\n';
                    l_bufferLength++;
                }

                l_isFirstPass = false;

            PARSE_EXIT:
                free( l_trimmedText );
            }

            printf( "LINE %s\n", l_line );
            l_line = strtok( NULL, l_delimiter );
        }

        if ( l_bufferLength == 0 ) {
            free( l_buffer );
        }

        if ( l_bufferLength ) {
            l_buffer = ( char* )realloc( l_buffer, ( l_bufferLength + 1 ) );

            l_buffer[ l_bufferLength ] = '\0';

            insertIntoArray( ( void*** )&l_overlay, strdup( l_buffer ) );

            free( l_buffer );

            l_bufferLength = 0;
        }

        free( l_text );

        for ( size_t i = 1; i < ( arrayLength( l_labels ) + 1 ); i++ ) {
            free( l_labels[ i ] );
        }

        free( l_labels );
        free( l_labelCounts );
    }

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

                    const size_t l_labelLength =
                        concatBeforeAndAfterString( &l_label, "[", "]" );
                    printf( "LABEL2 %u %s\n", l_labelLength, l_label );

#if 1
                    // Register for rendering
                    {
                        char*** l_itemSettings;

                        if ( _useCallback( "core$getSettingsContentByLabel",
                                           &l_itemSettings, l_label ) != 0 ) {
                            char* l_itemDefaultSettings;

                            const size_t l_itemDefaultSettingsIndex =
                                _findStringInArray( l_overlayLabels, l_label );

                            if ( l_itemDefaultSettingsIndex >= 1 ) {
                                l_itemDefaultSettings =
                                    l_overlay[ l_itemDefaultSettingsIndex ];
                            }

                            _useCallback( "core$readSettingsFromString",
                                          l_itemDefaultSettings );

                            _useCallback( "core$getSettingsContentByLabel",
                                          &l_itemSettings, "keyboard" );

                            printf( "TRUE\n" );

                        } else {
                            printf( "FALSE\n" );
                        }
                    }
#endif

                    free( l_itemIndexAsText );
                }

                free( l_overlayNameWithUnderscore );
            }

            free( l_label );
        }

        free( l_itemNames );
        free( l_itemCounts );
    }

    {
        char** l_content = l_overlayLabels;
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

        free( l_overlayLabels );
    }

    {
        char** l_content = l_overlay;
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

        free( l_overlay );
    }
}

uint16_t overlayRegister( const char* _overlayName,
                          const char* const* _overlayItems,
                          const char* _overlay,
                          uintptr_t* _overlayValueReferences,
                          const char* _overlayDefaultHotkey ) {
    uint16_t l_returnValue = 0;

    parseLayout( _overlayName, _overlayItems, _overlay );

    return ( l_returnValue );
}
