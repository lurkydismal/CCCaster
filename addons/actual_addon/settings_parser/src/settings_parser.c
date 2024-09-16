#include "settings_parser.h"

#include <ctype.h>
#include <omp.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stdfunc.h"

#define ENOKEY 126

typedef enum { KEY, VALUE } content_t;

static char** g_labels;
static char**** g_content;
/*
 * [ // [ labels ]
 *  [ // [ pairs ]
 *   [ // [ 2 ] - key : value
 *    "key",
 *    "value"
 *   ]
 *  ]
 * ]
 */

static inline size_t* getLabelCount( void ) {
    static size_t l_labelCount = 0;

    return ( &l_labelCount );
}

static inline size_t* getLabelKeysCountByIndex( const size_t _labelIndex ) {
    return ( ( size_t* )( &( g_content[ _labelIndex ][ 0 ][ 0 ] ) ) );
}

static size_t getLabelIndex( const char* _label ) {
    const size_t* l_labelCount = getLabelCount();
    size_t l_labelIndex = UINT32_MAX;

    for ( size_t _index = 0; _index < *l_labelCount; _index++ ) {
        if ( strcmp( _label, g_labels[ _index ] ) == 0 ) {
            l_labelIndex = _index;

            break;
        }
    }

    return ( l_labelIndex );
}

static void freeLabelByIndex( const size_t _labelIndex ) {
    size_t* l_labelCount = getLabelCount();

    if ( !( *l_labelCount ) ) {
        return;
    }

    const size_t l_keysCount = *getLabelKeysCountByIndex( _labelIndex );

    free( g_labels[ _labelIndex ] );

#pragma omp simd
    for ( size_t _keyIndex = 1; _keyIndex < l_keysCount; _keyIndex++ ) {
        free( g_content[ _labelIndex ][ _keyIndex ][ 0 ] );

        free( g_content[ _labelIndex ][ _keyIndex ][ 1 ] );

        free( g_content[ _labelIndex ][ _keyIndex ] );
    }

    free( g_content[ _labelIndex ][ 0 ] );
    free( g_content[ _labelIndex ] );

    ( *l_labelCount )--;

    for ( size_t _index = _labelIndex; _index < *l_labelCount; _index++ ) {
        g_labels[ _index ] = g_labels[ _index + 1 ];
        g_content[ _index ] = g_content[ _index + 1 ];
    }

    if ( *l_labelCount ) {
        g_labels =
            ( char** )realloc( g_labels, *l_labelCount * sizeof( char* ) );
        g_content =
            ( char**** )realloc( g_content, *l_labelCount * sizeof( char*** ) );
    }
#if 0
    else {
        free( g_labels );
        free( g_content );
    }
#endif
}

static /* inline */ size_t freeLabelByLabel( const char* _label ) {
    const size_t l_labelIndex = getLabelIndex( _label );

    if ( l_labelIndex != UINT32_MAX ) {
        freeLabelByIndex( l_labelIndex );
    }

    return ( l_labelIndex );
}

static void addLabel( const char* _text ) {
    size_t* l_labelCount = getLabelCount();

    const size_t l_isFreed = ( freeLabelByLabel( _text ) != UINT32_MAX );

    ( *l_labelCount )++;
    const size_t l_labelIndex = ( *l_labelCount - 1 );

    g_labels = ( char** )realloc( g_labels, *l_labelCount * sizeof( char* ) );

    g_labels[ l_labelIndex ] = strdup( _text );

    g_content =
        ( char**** )realloc( g_content, *l_labelCount * sizeof( char*** ) );

    g_content[ l_labelIndex ] = ( char*** )malloc( 1 * sizeof( char** ) );
    g_content[ l_labelIndex ][ 0 ] = ( char** )malloc( 1 * sizeof( char* ) );

    ( *getLabelKeysCountByIndex( l_labelIndex ) ) = 1;
}

static /* inline */ void changeKeyByIndex( const size_t _keyIndex,
                                           const size_t _labelIndex,
                                           const char* _value ) {
    char* l_value = strdup( _value );

    free( g_content[ _labelIndex ][ _keyIndex ][ 1 ] );

    g_content[ _labelIndex ][ _keyIndex ][ 1 ] = l_value;
}

static char*** getContentByIndex( const size_t _labelIndex ) {
    char*** l_content = g_content[ _labelIndex ];
    const size_t l_contentLength = ( size_t )( l_content[ 0 ][ 0 ] );

    char*** l_returnValue =
        ( char*** )malloc( l_contentLength * sizeof( char** ) );

    l_returnValue[ 0 ] = ( char** )malloc( 1 * sizeof( char* ) );

    ( *( size_t* )( &( l_returnValue[ 0 ][ 0 ] ) ) ) = l_contentLength;

#pragma omp simd
    for ( size_t _index = 1; _index < l_contentLength; _index++ ) {
        l_returnValue[ _index ] = ( char** )malloc( 2 * sizeof( char* ) );

        l_returnValue[ _index ][ 0 ] = strdup( l_content[ _index ][ 0 ] );
        l_returnValue[ _index ][ 1 ] = strdup( l_content[ _index ][ 1 ] );
    }

    return ( l_returnValue );
}

static void addContent( const char* _text, const content_t _contentType ) {
    const size_t* l_labelCount = getLabelCount();

    const size_t l_labelIndex = ( *l_labelCount - 1 );
    const size_t l_keysCount =
        ( size_t )( g_content[ l_labelIndex ][ 0 ][ 0 ] );

    const size_t l_keyIndex = ( l_keysCount - 1 );

    char** l_content =
        &( g_content[ l_labelIndex ][ l_keyIndex ][ _contentType ] );

    *l_content = strdup( _text );
}

static size_t getKeyIndex( char*** _content, const char* _key ) {
    const size_t l_contentKeyCount = ( size_t )( _content[ 0 ][ 0 ] );
    size_t l_keyIndex = UINT32_MAX;

    for ( size_t _index = 1; _index < l_contentKeyCount; _index++ ) {
        const char* l_key = _content[ _index ][ 0 ];

        if ( strcmp( _key, l_key ) == 0 ) {
            l_keyIndex = _index;

            break;
        }
    }

    return ( l_keyIndex );
}

static void addKey( const char* _text ) {
    const size_t* l_labelCount = getLabelCount();

    const size_t l_labelIndex = ( *l_labelCount - 1 );

    ( *( size_t* )( &( g_content[ l_labelIndex ][ 0 ][ 0 ] ) ) )++;

    const size_t l_keysCount =
        ( size_t )( g_content[ l_labelIndex ][ 0 ][ 0 ] );
    const size_t l_keyIndex = ( l_keysCount - 1 );

    g_content[ l_labelIndex ] = ( char*** )realloc(
        g_content[ l_labelIndex ], l_keysCount * sizeof( char** ) );
    g_content[ l_labelIndex ][ l_keyIndex ] =
        ( char** )malloc( 2 * sizeof( char* ) );

    addContent( _text, KEY );
}

static inline void addValue( const char* _text ) {
    addContent( _text, VALUE );
}

static /* inline */ char* trim( const char* _text ) {
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

static void parseLine( char* _text ) {
    char* l_trimmedText = trim( _text );
    const size_t l_textLength = strlen( l_trimmedText );

    if ( !l_textLength ) {
        goto EXIT;
    }

    if ( l_trimmedText[ 0 ] == '[' ) {
        l_trimmedText[ l_textLength - 1 ] = '\0';
        char* l_label = ( l_trimmedText + 1 );

        addLabel( l_label );

    } else {
        for ( size_t _index = 0; _index < ( l_textLength - 1 ); _index++ ) {
            if ( l_trimmedText[ _index ] == '=' ) {
                l_trimmedText[ _index ] = '\0';

                const char* l_key = l_trimmedText;
                const char* l_value = ( l_trimmedText + _index + 1 );

                addKey( l_key );
                addValue( l_value );

                break;
            }
        }
    }

EXIT:
    free( l_trimmedText );
}

/* inline */ char*** getSettingsContentByLabel( const char* _label ) {
    const size_t l_labelIndex = getLabelIndex( _label );

    if ( l_labelIndex == UINT32_MAX ) {
        return ( NULL );
    }

    return ( getContentByIndex( l_labelIndex ) );
}

uint16_t changeSettingsKeyByLabel( const char* _key,
                                   const char* _label,
                                   const char* _value ) {
    uint16_t l_returnValue = 0;

    char*** l_content = getSettingsContentByLabel( _label );

    if ( l_content == NULL ) {
        l_returnValue = 1;

        goto EXIT;
    }

    {
        const size_t l_keyIndex = getKeyIndex( l_content, _key );

        size_t l_keysCount = ( size_t )( l_content[ 0 ][ 0 ] );

#pragma omp simd
        for ( size_t _keyIndex = 1; _keyIndex < l_keysCount; _keyIndex++ ) {
            free( l_content[ _keyIndex ][ 0 ] );

            free( l_content[ _keyIndex ][ 1 ] );

            free( l_content[ _keyIndex ] );
        }

        free( l_content[ 0 ] );
        free( l_content );

        const size_t l_labelIndex = getLabelIndex( _label );

        if ( l_keyIndex == UINT32_MAX ) {
            l_returnValue = ENOKEY;

            size_t* l_labelCount = getLabelCount();

            const size_t l_labelCountBackup = *l_labelCount;
            *l_labelCount = ( l_labelIndex + 1 );

            addKey( _key );
            addValue( _value );

            *l_labelCount = l_labelCountBackup;

            goto WRITE;
        }

        {
            // Label does exist and key does exist
            // Change key in label
            changeKeyByIndex( l_keyIndex, l_labelIndex, _value );
        }
    }
WRITE:
    writeSettingsToFile( SETTINGS_FILE_PATH );

EXIT:
    return ( l_returnValue );
}

uint16_t readSettingsFromFile( const char* _fileName ) {
    uint16_t l_returnValue = 1;

    FILE* l_fileHandle = fopen( _fileName, "r" );

    if ( l_fileHandle ) {
        char l_line[ MAX_LINE_LENGTH ];

        while ( fgets( l_line, MAX_LINE_LENGTH, l_fileHandle ) != NULL ) {
            parseLine( l_line );
        }

        l_returnValue = 0;

        fclose( l_fileHandle );
    }

    return ( l_returnValue );
}

uint16_t readSettingsFromString( const char* _text ) {
    uint16_t l_returnValue = 1;

    char* l_text = strdup( _text );

    const char l_delimiter[] = "\n";
    char* l_line = strtok( l_text, l_delimiter );

    while ( l_line ) {
        parseLine( l_line );

        l_line = strtok( NULL, l_delimiter );
    }

    free( l_text );

    l_returnValue = writeSettingsToFile( SETTINGS_BACKUP_FILE_PATH );

    return ( l_returnValue );
}

uint16_t writeSettingsToFile( const char* _fileName ) {
    uint16_t l_returnValue = 1;

    FILE* l_fileHandle = fopen( _fileName, "w" );

    if ( l_fileHandle ) {
        const size_t l_labelCount = *( getLabelCount() );

        for ( size_t _labelIndex = 0; _labelIndex < l_labelCount;
              _labelIndex++ ) {
            const char* l_label = g_labels[ _labelIndex ];
            const size_t l_labelLength = strlen( l_label );
            const bool l_isNotFirstLabel = ( _labelIndex );
            const size_t l_labelBufferLength =
                ( 1 + l_labelLength + 2 + ( l_isNotFirstLabel ) );
            char* l_buffer = ( char* )malloc( l_labelBufferLength + 1 );

            if ( l_isNotFirstLabel ) {
                l_buffer[ 0 ] = '\n';
            }

            l_buffer[ 0 + ( l_isNotFirstLabel ) ] = '[';
            memcpy( ( l_buffer + 1 + ( l_isNotFirstLabel ) ), l_label,
                    l_labelLength );
            l_buffer[ l_labelLength + 1 + ( l_isNotFirstLabel ) ] = ']';
            l_buffer[ l_labelLength + 2 + ( l_isNotFirstLabel ) ] = '\n';
            l_buffer[ l_labelBufferLength ] = '\0';

            fwrite( l_buffer, sizeof( l_buffer[ 0 ] ), l_labelBufferLength,
                    l_fileHandle );

            free( l_buffer );

            size_t l_keysCount =
                ( size_t )( g_content[ _labelIndex ][ 0 ][ 0 ] );

            for ( size_t _keyIndex = 1; _keyIndex < l_keysCount; _keyIndex++ ) {
                const char* l_key = g_content[ _labelIndex ][ _keyIndex ][ 0 ];
                const size_t l_keyLength = strlen( l_key );
                const size_t l_keyBufferLength = ( l_keyLength + 2 );
                l_buffer = ( char* )malloc( l_keyBufferLength + 1 );
                memcpy( l_buffer, l_key, l_keyLength );
                l_buffer[ l_keyLength ] = ' ';
                l_buffer[ l_keyLength + 1 ] = '=';
                l_buffer[ l_keyBufferLength ] = '\0';

                fwrite( l_buffer, sizeof( l_buffer[ 0 ] ), l_keyBufferLength,
                        l_fileHandle );

                free( l_buffer );

                const char* l_value =
                    g_content[ _labelIndex ][ _keyIndex ][ 1 ];
                const size_t l_valueLength = strlen( l_value );
                const size_t l_valueBufferLength = ( 1 + l_valueLength + 1 );
                l_buffer = ( char* )malloc( l_valueBufferLength + 1 );
                l_buffer[ 0 ] = ' ';
                memcpy( ( l_buffer + 1 ), l_value, l_valueLength );
                l_buffer[ l_valueLength + 1 ] = '\n';
                l_buffer[ l_valueBufferLength ] = '\0';

                fwrite( l_buffer, sizeof( l_buffer[ 0 ] ), l_valueBufferLength,
                        l_fileHandle );

                free( l_buffer );
            }
        }

        l_returnValue = 0;

        fclose( l_fileHandle );
    }

    return ( l_returnValue );
}

/* inline */ uint16_t freeSettingsTable( void ) {
    uint16_t l_returnValue = 1;

#pragma omp simd
    for ( size_t _labelCount = *( getLabelCount() ); _labelCount > 0;
          _labelCount-- ) {
        freeLabelByIndex( _labelCount - 1 );
    }

    if ( g_labels ) {
        free( g_labels );
    }

    if ( g_content ) {
        free( g_content );
    }

    if ( ( !g_labels ) && ( !g_content ) && ( !( *getLabelCount() ) ) ) {
        l_returnValue = 0;
    }

    return ( l_returnValue );
}

void printSettings( void ) {
    const size_t l_labelCount = *( getLabelCount() );

    for ( size_t _labelIndex = 0; _labelIndex < l_labelCount; _labelIndex++ ) {
        const char* l_label = g_labels[ _labelIndex ];
        char*** l_content = g_content[ _labelIndex ];
        const size_t l_keysCount = arrayLength( l_content[ 0 ] );
        char*** l_contentFirstElement = arrayFirstElementPointer( l_content );
        char** const* l_contentEnd = ( l_contentFirstElement + l_keysCount );

        printf( "[%s] : %d %p\n", l_label, l_keysCount, l_contentEnd );

        for ( char*** _elementPointer = l_contentFirstElement;
              _elementPointer != l_contentEnd; _elementPointer++ ) {
            const char* l_key = ( *_elementPointer )[ 0 ];
            const char* l_value = ( *_elementPointer )[ 1 ];

            printf( "%s=%s\n", l_key, l_value );
        }
    }
}
