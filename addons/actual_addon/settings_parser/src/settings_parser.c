#include "settings_parser.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ENOKEY 126

typedef enum { KEY, VALUE } content_t;

static char** g_labels;
static size_t g_labelCount = 0;
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

size_t* getLabelCount( void ) {
    return ( &g_labelCount );
}

static size_t getLabelIndex( const char* _label ) {
    const size_t* l_labelCount = getLabelCount();

    for ( size_t _index = 0; _index < *l_labelCount; _index++ ) {
        if ( strcmp( _label, g_labels[ _index ] ) == 0 ) {
            return ( _index );
        }
    }

    return ( UINT32_MAX );
}

static void freeLabelByIndex( const size_t _labelIndex ) {
    size_t* l_labelCount = getLabelCount();

    if ( !*l_labelCount ) {
        return;
    }

    size_t l_keysCount = ( size_t )( g_content[ _labelIndex ][ 0 ][ 0 ] );

    free( g_labels[ _labelIndex ] );

    for ( size_t _keyIndex = 1; _keyIndex < l_keysCount; _keyIndex++ ) {
        free( g_content[ _labelIndex ][ _keyIndex ][ 0 ] );

        free( g_content[ _labelIndex ][ _keyIndex ][ 1 ] );

        free( g_content[ _labelIndex ][ _keyIndex ] );
    }

    free( g_content[ _labelIndex ][ 0 ] );
    free( g_content[ _labelIndex ] );

    ( *l_labelCount )--;

    if ( !*l_labelCount ) {
        free( g_labels );
        free( g_content );
    }
}

static void freeLabelByLabel( const char* _label ) {
    const size_t l_labelIndex = getLabelIndex( _label );

    if ( l_labelIndex != UINT32_MAX ) {
        freeLabelByIndex( l_labelIndex );
    }
}

static void addLabel( const char* _text ) {
    size_t* l_labelCount = getLabelCount();

    ( *l_labelCount )++;
    const size_t l_labelIndex = ( *l_labelCount - 1 );

    g_labels = ( char** )realloc( g_labels, *l_labelCount * sizeof( char* ) );

    g_labels[ l_labelIndex ] = strdup( _text );

    g_content =
        ( char**** )realloc( g_content, *l_labelCount * sizeof( char*** ) );

    g_content[ l_labelIndex ] = ( char*** )malloc( 1 * sizeof( char** ) );
    g_content[ l_labelIndex ][ 0 ] = ( char** )malloc( 1 * sizeof( char* ) );

    ( *( size_t* )( &( g_content[ l_labelIndex ][ 0 ][ 0 ] ) ) ) = 1;
}

static void changeKeyByIndex( const size_t _keyIndex,
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

    for ( size_t _index = 1; _index < l_contentKeyCount; _index++ ) {
        const char* l_key = _content[ _index ][ 0 ];

        if ( strcmp( _key, l_key ) == 0 ) {
            return ( _index );
        }
    }

    return ( UINT32_MAX );
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

static void addValue( const char* _text ) {
    addContent( _text, VALUE );
}

static char* trim( const char* _text ) {
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
    size_t l_textLength = strlen( _text );

    if ( _text[ 0 ] == '[' ) {
        _text[ l_textLength - 1 ] = '\0';
        char* l_label = ( _text + 1 );

        const size_t l_labelIndex = getLabelIndex( l_label );

        if ( l_labelIndex != UINT32_MAX ) {
            freeLabelByIndex( l_labelIndex );
        }

        addLabel( l_label );

    } else {
        for ( size_t _index = 0; _index < ( l_textLength - 1 ); _index++ ) {
            if ( _text[ _index ] == '=' ) {
                _text[ _index ] = '\0';

                const char* l_key = _text;
                const char* l_value = ( _text + _index + 1 );

                addKey( l_key );
                addValue( l_value );

                break;
            }
        }
    }
}

char*** getSettingsContentByLabel( const char* _label ) {
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

    const size_t l_keyIndex = getKeyIndex( l_content, _key );

    size_t l_keysCount = ( size_t )( l_content[ 0 ][ 0 ] );

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

        goto EXIT;
    }

    changeKeyByIndex( l_keyIndex, l_labelIndex, _value );

EXIT:
    return ( l_returnValue );
}

uint16_t readSettingsFromFile( const char* _fileName ) {
    uint16_t l_returnValue = 1;

    FILE* l_fileHandle = fopen( _fileName, "r" );

    if ( l_fileHandle ) {
        char l_line[ MAX_LINE_LENGTH ];

        while ( fgets( l_line, MAX_LINE_LENGTH, l_fileHandle ) != NULL ) {
            char* l_trimmedLine = trim( l_line );
            const size_t l_lineLength = strlen( l_trimmedLine );

            if ( l_lineLength ) {
                parseLine( l_trimmedLine );
            }

            free( l_trimmedLine );
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
        char* l_trimmedLine = trim( l_line );
        const size_t l_lineLength = strlen( l_trimmedLine );

        if ( l_lineLength ) {
            parseLine( l_trimmedLine );
        }

        free( l_trimmedLine );

        l_line = strtok( NULL, l_delimiter );
    }

    free( l_text );

    l_returnValue = 0;

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
                ( l_labelLength + 3 + ( l_isNotFirstLabel ) );
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
                const size_t l_valueBufferLength = ( l_valueLength + 2 );
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

uint16_t freeSettingsTable( void ) {
    uint16_t l_returnValue = 1;

    const size_t* l_labelCount = getLabelCount();

#if 0
    while ( *l_labelCount ) {
        freeLabelByIndex( 0 );
    }
#endif

    const size_t l_labelCountBackup = *l_labelCount;

    for ( size_t _labelIndex = 0; _labelIndex < l_labelCountBackup;
          _labelIndex++ ) {
        freeLabelByIndex( _labelIndex );
    }

    if ( ( !g_labels ) && ( !g_content ) && ( !*l_labelCount ) ) {
        l_returnValue = 0;
    }

    return ( l_returnValue );
}

void printSettings( void ) {
    const size_t l_labelCount = *( getLabelCount() );

    for ( size_t _labelIndex = 0; _labelIndex < l_labelCount; _labelIndex++ ) {
        const size_t l_keysCount =
            ( size_t )( g_content[ _labelIndex ][ 0 ][ 0 ] );

        for ( size_t _keyIndex = 1; _keyIndex < l_keysCount; _keyIndex++ ) {
            const char* l_label = g_labels[ _labelIndex ];
            char*** l_content = g_content[ _labelIndex ];
            const char* l_key = l_content[ _keyIndex ][ 0 ];
            const char* l_value = l_content[ _keyIndex ][ 1 ];

            printf( "[%s]\n%s=%s\n", l_label, l_key, l_value );
        }
    }
}
