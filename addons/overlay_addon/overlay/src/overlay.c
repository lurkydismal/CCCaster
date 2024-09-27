#include "overlay.h"

#include "_useCallback.h"
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
    char** l_overlay = ( char** )createArray( sizeof( char* ) );

    {
        char* l_text = strdup( _overlay );

        const char l_delimiter[] = "\n";
        char* l_line = strtok( l_text, l_delimiter );
        char* l_buffer;
        size_t l_bufferLength = 0;

        while ( l_line ) {
            {
                char* l_trimmedText = trim( l_line );
                const size_t l_textLength = strlen( l_trimmedText );

                if ( !l_textLength ) {
                    goto PARSE_EXIT;
                }
                _useCallback( "log$transaction$query", "TEST3\n" );
                _useCallback( "log$transaction$commit" );

                l_buffer = ( char* )realloc(
                    l_buffer, ( l_bufferLength + l_textLength + 1 ) );
                _useCallback( "log$transaction$query", "TEST10\n" );
                _useCallback( "log$transaction$commit" );
                memcpy( ( l_buffer + l_bufferLength ), l_trimmedText,
                        l_textLength );
                _useCallback( "log$transaction$query", "TEST4\n" );
                _useCallback( "log$transaction$commit" );
                l_bufferLength += l_textLength;
                l_buffer[ l_bufferLength ] = '\n';
                l_bufferLength++;
                _useCallback( "log$transaction$query", "TEST5\n" );
                _useCallback( "log$transaction$commit" );
                _useCallback( "log$transaction$query", l_buffer );
                _useCallback( "log$transaction$query", "\n" );
                _useCallback( "log$transaction$commit" );

#if 0
                if ( l_trimmedText[ 0 ] == '[' ) {
                    l_buffer[ l_bufferLength ] = '\0';

                    insertIntoArray( ( void*** )&l_overlay,
                                     strdup( l_buffer ) );
                    _useCallback( "log$transaction$query", l_buffer );
                    _useCallback( "log$transaction$query", "\n" );
                    _useCallback( "log$transaction$commit" );

                    free( l_buffer );

                    l_bufferLength = 0;
                }
#endif

            PARSE_EXIT:
                free( l_trimmedText );
            }

            l_line = strtok( NULL, l_delimiter );
        }

        free( l_text );
    }

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
        size_t l_itemCount;

        const size_t l_itemNameIndex =
            _findStringInArray( l_itemNames, *_item );

        if ( l_itemNameIndex >= 0 ) {
            ( l_itemCounts[ l_itemNameIndex ] )++;

            l_itemCount = l_itemCounts[ l_itemNameIndex ];

        } else {
            insertIntoArray( ( void*** )&l_itemNames, ( void* )( *_item ) );
            insertIntoArray( ( void*** )&l_itemCounts, ( void* )1 );
        }

        const size_t l_itemIndex = ( l_itemCount - 1 );

        {
            char* l_buffer = strdup( *_item );

            {
                const size_t l_overlayNameLength = strlen( _overlayName );
                char* l_overlayNameWithUnderscore = ( char* )malloc(
                    ( l_overlayNameLength + 1 + 1 ) * sizeof( char ) );

                memcpy( l_overlayNameWithUnderscore, _overlayName,
                        l_overlayNameLength );

                l_overlayNameWithUnderscore[ l_overlayNameLength ] = '_';
                l_overlayNameWithUnderscore[ l_overlayNameLength + 1 ] = '\0';

                char* l_itemIndexAsText = stoa( l_itemIndex );

                concatBeforeAndAfterString(
                    &l_buffer, l_overlayNameWithUnderscore, l_itemIndexAsText );

                free( l_overlayNameWithUnderscore );
                free( l_itemIndexAsText );
            }

            _useCallback( "log$transaction$query", l_buffer );
            _useCallback( "log$transaction$query", "\n" );

#if 0
            if ( _useCallback( "core$getSettingsContentByLabel", &g_settings,
                        "keyboard" ) != 0 ) {
                _useCallback( "log$transaction$query", "TRUE\n" );

           } else {
                _useCallback( "log$transaction$query", "FALSE\n" );
           }
#endif

            free( l_buffer );
        }
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
